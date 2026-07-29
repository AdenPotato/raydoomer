#pragma once

#include "platform/audio_device.h"

namespace platform {

/// raylib-backed audio device.
///
/// @remarks
/// Owns device acquisition and playback dispatch only. Loading sounds into
/// handles belongs to the resource system and arrives with the WAD loader; until
/// then no handle resolves and `playSound` is a reported no-op rather than a
/// crash. Audio failing is never fatal - the game decides how to respond, and
/// running silent is a valid response.
class RaylibAudioDevice final : public AudioDevice {
public:
    ~RaylibAudioDevice() override;

    bool open() override;
    void close() override;
    void playSound(SoundId id, float volume) override;

private:
    bool isOpen_ = false;
};

} // namespace platform
