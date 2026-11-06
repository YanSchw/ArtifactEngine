#include "WebAudioDevice.h"

#include <algorithm>
#include <emscripten.h>

EM_JS(int, ArtifactWebAudioOpen, (int InChannels), {
    var Context = globalThis.AudioContext || globalThis.webkitAudioContext;
    if (!Context) {
        return 0;
    }
    var audio = { context: new Context({ latencyHint: 'interactive' }), channels: InChannels, scheduled: 0 };
    audio.events = ['pointerdown', 'mousedown', 'keydown', 'touchend'];
    audio.resume = function() {
        if (audio.context.state !== 'running') {
            audio.context.resume();
        }
    };
    audio.events.forEach(function(name) { window.addEventListener(name, audio.resume, true); });
    globalThis.ArtifactWebAudio = audio;
    return audio.context.sampleRate;
});

EM_JS(int, ArtifactWebAudioFramesWanted, (int InLeadFrames), {
    var audio = globalThis.ArtifactWebAudio;
    if (!audio || audio.context.state !== 'running') {
        return 0;
    }
    var now = audio.context.currentTime;
    if (audio.scheduled < now) {
        audio.scheduled = now + 0.01;
    }
    return Math.max(0, Math.ceil(InLeadFrames - (audio.scheduled - now) * audio.context.sampleRate));
});

EM_JS(void, ArtifactWebAudioSubmit, (const float* InSamples, int InFrames), {
    var audio = globalThis.ArtifactWebAudio;
    var buffer = audio.context.createBuffer(audio.channels, InFrames, audio.context.sampleRate);
    var first = InSamples >> 2;
    for (var channel = 0; channel < audio.channels; channel++) {
        var data = buffer.getChannelData(channel);
        for (var frame = 0; frame < InFrames; frame++) {
            data[frame] = HEAPF32[first + frame * audio.channels + channel];
        }
    }
    var source = audio.context.createBufferSource();
    source.buffer = buffer;
    source.connect(audio.context.destination);
    source.start(audio.scheduled);
    audio.scheduled += InFrames / audio.context.sampleRate;
});

EM_JS(void, ArtifactWebAudioClose, (), {
    var audio = globalThis.ArtifactWebAudio;
    if (!audio) {
        return;
    }
    audio.events.forEach(function(name) { window.removeEventListener(name, audio.resume, true); });
    audio.context.close();
    delete globalThis.ArtifactWebAudio;
});

WebAudioDevice::~WebAudioDevice() {
    Close();
}

bool WebAudioDevice::Open(const AudioDeviceDesc& InDesc, const AudioRenderCallback& InRender) {
    Close();

    const int sampleRate = ArtifactWebAudioOpen((int)InDesc.Channels);
    if (sampleRate <= 0) {
        return false;
    }

    m_Format.SampleRate = (uint32_t)sampleRate;
    m_Format.Channels = InDesc.Channels;
    m_Format.SampleFormat = AudioSampleFormat::Float32;
    m_Render = InRender;
    m_BlockFrames = std::max(InDesc.BufferFrames, 128u);
    m_LeadFrames = std::max(m_BlockFrames * 2, m_Format.SampleRate / 10);
    m_Open = true;
    return true;
}

void WebAudioDevice::Close() {
    if (m_Open) {
        ArtifactWebAudioClose();
        m_Open = false;
    }
}

void WebAudioDevice::Pump() {
    if (!m_Open) {
        return;
    }
    const uint32_t wanted = (uint32_t)ArtifactWebAudioFramesWanted((int)m_LeadFrames);
    if (wanted == 0) {
        return;
    }
    const uint32_t frames = std::min((wanted + m_BlockFrames - 1) / m_BlockFrames * m_BlockFrames, m_LeadFrames * 2);
    m_Samples.Resize((size_t)frames * m_Format.Channels);
    Render(m_Samples.Data(), frames);
    ArtifactWebAudioSubmit(m_Samples.Data(), (int)frames);
}
