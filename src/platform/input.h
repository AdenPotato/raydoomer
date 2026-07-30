#pragma once

namespace platform {

/// Logical actions, not physical keys.
///
/// @remarks
/// Deliberately named for what the player is doing rather than which key is
/// pressed. Rebinding then lives entirely in the backend, and gameplay never
/// learns that "forward" happens to be W. Engine code carries no game nouns
/// (engine_protocol.md), so these stay generic verbs.
enum class Key {
    Forward,
    Back,
    Left,
    Right,
    Jump,
    Fire,
    Interact,

    // Development controls. Kept alongside game actions so there is one input
    // path rather than two, but deliberately grouped: these are not things the
    // player does.
    DebugView,       ///< Toggle the debug view entirely.
    DebugCycleMode,  ///< Cycle wireframe / solid / both.
    DebugOverlay,    ///< Toggle the solver overlay.
};

/// Relative mouse movement since the previous poll, in pixels.
struct MouseDelta {
    float dx = 0.0f;
    float dy = 0.0f;

    friend bool operator==(const MouseDelta&, const MouseDelta&) = default;
};

/// Input state, sampled once per frame.
///
/// @remarks
/// `poll` defines the sampling point. Everything read afterwards reflects that
/// instant, so two systems in the same tick cannot observe different input -
/// which would be a determinism bug.
class Input {
public:
    virtual ~Input() = default;

    /// Samples the current input state. Call once per frame, before reading.
    virtual void poll() = 0;

    /// @returns true if the action is currently held.
    virtual bool isKeyDown(Key key) const = 0;

    /// @returns Mouse movement since the previous `poll`.
    virtual MouseDelta mouseDelta() const = 0;
};

} // namespace platform
