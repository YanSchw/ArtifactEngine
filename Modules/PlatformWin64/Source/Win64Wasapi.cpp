#include "Win64Wasapi.h"

#include <windows.h>
#include <mmdeviceapi.h>
#include <audioclient.h>
#include <mmreg.h>
#include <avrt.h>

#include <atomic>
#include <future>
#include <thread>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "avrt.lib")

static const GUID s_FloatSubFormat = { 0x00000003, 0x0000, 0x0010, { 0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71 } };

struct Win64WasapiStream::State {
    uint32_t Channels = 2;
    uint32_t BufferFrames = 512;
    RenderFunction Render;
    std::atomic<uint32_t> SampleRate = 0;
    std::atomic<bool> Running = false;
    std::thread Thread;
};

template<typename T>
static void SafeRelease(T*& OutObject) {
    if (OutObject) {
        OutObject->Release();
        OutObject = nullptr;
    }
}

struct WasapiSession {
    IMMDevice* Device = nullptr;
    IAudioClient* Client = nullptr;
    IAudioRenderClient* RenderClient = nullptr;
    HANDLE Event = nullptr;
    UINT32 BufferFrames = 0;
    uint32_t SampleRate = 0;

    bool Open(uint32_t InChannels, uint32_t InBufferFrames) {
        IMMDeviceEnumerator* enumerator = nullptr;
        if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL, __uuidof(IMMDeviceEnumerator), (void**)&enumerator))) {
            return false;
        }
        const HRESULT found = enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &Device);
        enumerator->Release();
        if (FAILED(found) || FAILED(Device->Activate(__uuidof(IAudioClient), CLSCTX_ALL, nullptr, (void**)&Client))) {
            return false;
        }

        WAVEFORMATEX* mixFormat = nullptr;
        if (FAILED(Client->GetMixFormat(&mixFormat))) {
            return false;
        }
        SampleRate = mixFormat->nSamplesPerSec;
        CoTaskMemFree(mixFormat);

        WAVEFORMATEXTENSIBLE format = {};
        format.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
        format.Format.nChannels = (WORD)InChannels;
        format.Format.nSamplesPerSec = SampleRate;
        format.Format.wBitsPerSample = 32;
        format.Format.nBlockAlign = (WORD)(InChannels * sizeof(float));
        format.Format.nAvgBytesPerSec = SampleRate * format.Format.nBlockAlign;
        format.Format.cbSize = sizeof(WAVEFORMATEXTENSIBLE) - sizeof(WAVEFORMATEX);
        format.Samples.wValidBitsPerSample = 32;
        format.dwChannelMask = InChannels == 1 ? SPEAKER_FRONT_CENTER : SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
        format.SubFormat = s_FloatSubFormat;

        const REFERENCE_TIME duration = (REFERENCE_TIME)InBufferFrames * 2 * 10000000 / SampleRate;
        const DWORD flags = AUDCLNT_STREAMFLAGS_EVENTCALLBACK | AUDCLNT_STREAMFLAGS_AUTOCONVERTPCM | AUDCLNT_STREAMFLAGS_SRC_DEFAULT_QUALITY;
        if (FAILED(Client->Initialize(AUDCLNT_SHAREMODE_SHARED, flags, duration, 0, (WAVEFORMATEX*)&format, nullptr))) {
            return false;
        }

        Event = CreateEventW(nullptr, FALSE, FALSE, nullptr);
        return Event
            && SUCCEEDED(Client->SetEventHandle(Event))
            && SUCCEEDED(Client->GetBufferSize(&BufferFrames))
            && SUCCEEDED(Client->GetService(__uuidof(IAudioRenderClient), (void**)&RenderClient))
            && SUCCEEDED(Client->Start());
    }

    void Pump(Win64WasapiStream::State& InState) {
        while (InState.Running) {
            WaitForSingleObject(Event, 200);
            UINT32 padding = 0;
            if (FAILED(Client->GetCurrentPadding(&padding))) {
                return;
            }
            const UINT32 frames = BufferFrames - padding;
            if (frames == 0) {
                continue;
            }
            BYTE* data = nullptr;
            if (FAILED(RenderClient->GetBuffer(frames, &data))) {
                return;
            }
            InState.Render((float*)data, frames, SampleRate);
            RenderClient->ReleaseBuffer(frames, 0);
        }
    }

    void Close() {
        if (Client) {
            Client->Stop();
        }
        SafeRelease(RenderClient);
        SafeRelease(Client);
        SafeRelease(Device);
        if (Event) {
            CloseHandle(Event);
            Event = nullptr;
        }
    }
};

static void RunStream(Win64WasapiStream::State& InState, std::promise<bool>& OutStarted) {
    CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    DWORD taskIndex = 0;
    HANDLE task = AvSetMmThreadCharacteristicsW(L"Pro Audio", &taskIndex);

    bool reported = false;
    while (InState.Running) {
        WasapiSession session;
        const bool opened = session.Open(InState.Channels, InState.BufferFrames);
        if (opened) {
            InState.SampleRate = session.SampleRate;
        }
        if (!reported) {
            OutStarted.set_value(opened);
            reported = true;
        }
        if (opened) {
            session.Pump(InState);
        }
        session.Close();
        if (!opened && InState.SampleRate == 0) {
            break;
        }
        if (InState.Running) {
            Sleep(250);
        }
    }

    if (task) {
        AvRevertMmThreadCharacteristics(task);
    }
    CoUninitialize();
}

Win64WasapiStream::Win64WasapiStream() = default;

Win64WasapiStream::~Win64WasapiStream() {
    Stop();
}

bool Win64WasapiStream::Start(uint32_t InChannels, uint32_t InBufferFrames, const RenderFunction& InRender) {
    Stop();
    m_State = std::make_unique<State>();
    m_State->Channels = InChannels;
    m_State->BufferFrames = InBufferFrames;
    m_State->Render = InRender;
    m_State->Running = true;

    std::promise<bool> started;
    std::future<bool> result = started.get_future();
    m_State->Thread = std::thread([state = m_State.get(), started = std::move(started)]() mutable {
        RunStream(*state, started);
    });

    if (!result.get()) {
        Stop();
        return false;
    }
    return true;
}

void Win64WasapiStream::Stop() {
    if (!m_State) {
        return;
    }
    m_State->Running = false;
    if (m_State->Thread.joinable()) {
        m_State->Thread.join();
    }
    m_State.reset();
}

uint32_t Win64WasapiStream::GetSampleRate() const {
    return m_State ? m_State->SampleRate.load() : 0;
}
