#include "platform/raylib/raylib_audio_device.h"

#include <raylib.h>

namespace platform {

RaylibAudioDevice::~RaylibAudioDevice() {
    close();
}

bool RaylibAudioDevice::open() {
    if (isOpen_) {
        return true;
    }
    InitAudioDevice();
    isOpen_ = IsAudioDeviceReady();
    if (!isOpen_) {
        CloseAudioDevice();
    }
    return isOpen_;
}

void RaylibAudioDevice::close() {
    if (!isOpen_) {
        return;
    }
    CloseAudioDevice();
    isOpen_ = false;
}

void RaylibAudioDevice::playSound(SoundId id, float volume) {
    (void)id;
    (void)volume;
    if (!isOpen_) {
        return;
    }
    // No sound registry exists yet - loading WAD lumps into handles is the
    // resource system's job. Until then a handle resolves to nothing and this
    // reports rather than crashes: a stale handle is detected, never followed
    // (engine_protocol.md).
    TraceLog(LOG_DEBUG, "playSound: no sound registry yet, ignoring handle %u", id.value);
}

} // namespace platform
