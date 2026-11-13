#include "iOSAudioDevice.h"

#include <AudioToolbox/AudioToolbox.h>

static OSStatus RenderRemoteIO(void* InDevice, AudioUnitRenderActionFlags*, const AudioTimeStamp*, UInt32, UInt32 InFrameCount, AudioBufferList* OutData) {
    ((const iOSAudioDevice*)InDevice)->Render((float*)OutData->mBuffers[0].mData, InFrameCount);
    return noErr;
}

iOSAudioDevice::~iOSAudioDevice() {
    Close();
}

bool iOSAudioDevice::Open(const AudioDeviceDesc& InDesc, const AudioRenderCallback& InRender) {
    Close();

    AudioComponentDescription description = {};
    description.componentType = kAudioUnitType_Output;
    description.componentSubType = kAudioUnitSubType_RemoteIO;
    description.componentManufacturer = kAudioUnitManufacturer_Apple;

    AudioComponent component = AudioComponentFindNext(nullptr, &description);
    AudioUnit unit = nullptr;
    if (!component || AudioComponentInstanceNew(component, &unit) != noErr) {
        return false;
    }

    AudioStreamBasicDescription hardware = {};
    UInt32 size = sizeof(hardware);
    const bool hasHardwareRate = AudioUnitGetProperty(unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Output, 0, &hardware, &size) == noErr
                                 && hardware.mSampleRate > 0.0;

    m_Format.SampleRate = hasHardwareRate ? (uint32_t)hardware.mSampleRate : InDesc.SampleRate;
    m_Format.Channels = InDesc.Channels;
    m_Format.SampleFormat = AudioSampleFormat::Float32;
    m_Render = InRender;

    AudioStreamBasicDescription format = {};
    format.mSampleRate = m_Format.SampleRate;
    format.mFormatID = kAudioFormatLinearPCM;
    format.mFormatFlags = kAudioFormatFlagIsFloat | kAudioFormatFlagIsPacked;
    format.mFramesPerPacket = 1;
    format.mChannelsPerFrame = m_Format.Channels;
    format.mBitsPerChannel = 32;
    format.mBytesPerFrame = m_Format.GetBytesPerFrame();
    format.mBytesPerPacket = format.mBytesPerFrame;

    const AURenderCallbackStruct callback = { RenderRemoteIO, this };
    const bool configured = AudioUnitSetProperty(unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &format, sizeof(format)) == noErr
                            && AudioUnitSetProperty(unit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &callback, sizeof(callback)) == noErr;

    if (!configured || AudioUnitInitialize(unit) != noErr || AudioOutputUnitStart(unit) != noErr) {
        AudioComponentInstanceDispose(unit);
        return false;
    }
    m_Unit = unit;
    return true;
}

void iOSAudioDevice::Close() {
    if (!m_Unit) {
        return;
    }
    AudioUnit unit = (AudioUnit)m_Unit;
    AudioOutputUnitStop(unit);
    AudioUnitUninitialize(unit);
    AudioComponentInstanceDispose(unit);
    m_Unit = nullptr;
}
