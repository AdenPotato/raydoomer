#pragma once

#include <cstdint>

namespace platform {

/// An opaque handle to a loaded sound.
///
/// @remarks
/// Gameplay receives a handle it cannot dereference or free (engine_protocol.md).
/// A stale handle is detected and reported by the backend, never followed.
struct SoundId {
    uint32_t value = 0;

    friend bool operator==(const SoundId&, const SoundId&) = default;
};

/// The audio output device.
///
/// @remarks
/// Audio is presentation. Gameplay emits an event and a presentation handler
/// plays the sound; no entity calls this directly (presentation_protocol.md).
///
/// Lifetime: `open` acquires the device, `close` releases it. `close` is safe
/// after a failed `open`.
class AudioDevice {
public:
    virtual ~AudioDevice() = default;

    /// Acquires the audio device.
    /// @returns true on success. Audio is not fatal to the game: a failure here
    ///          means the game runs silent, and that is the game's decision to
    ///          make, not this layer's.
    virtual bool open() = 0;

    /// Releases the device. Safe to call more than once, and after a failed open.
    virtual void close() = 0;

    /// Requests playback of an already-loaded sound.
    /// @param id     Handle from the resource system.
    /// @param volume Linear gain in [0, 1].
    virtual void playSound(SoundId id, float volume) = 0;
};

} // namespace platform
