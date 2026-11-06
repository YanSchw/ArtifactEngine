#include "AudioDevice.h"
#include "Platform/Platform.h"

SharedObjectPtr<AudioDevice> AudioDevice::Create() {
    const Class deviceClass = Platform::GetDefaultAudioDeviceClass();
    SharedObjectPtr<AudioDevice> device = Object::Create<AudioDevice>(deviceClass);
    if (!device) {
        AE_WARN("No audio device '{0}' on this platform, audio stays silent", deviceClass.Name);
    }
    return device;
}
