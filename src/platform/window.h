#pragma once

namespace platform {

/// Window dimensions in pixels.
struct Size {
    int width = 0;
    int height = 0;

    friend bool operator==(const Size&, const Size&) = default;
};

/// The game window and its frame boundaries.
///
/// @remarks
/// The only tier permitted to touch the OS, a driver, or a device. Everything
/// above talks to this interface, which is what keeps the test suite runnable
/// with no window and no GPU (engine_protocol.md).
///
/// Lifetime: `open` acquires, `close` releases. `close` is always safe to call,
/// including when `open` failed - an early return that skips cleanup is a leak,
/// not a shortcut.
class Window {
public:
    virtual ~Window() = default;

    /// Creates the window and its graphics context.
    /// @returns true on success. On failure the window is not usable and
    ///          `close` is still safe to call.
    virtual bool open(int width, int height, const char* title) = 0;

    /// Releases the window and its context. Safe to call more than once, and
    /// safe to call after a failed `open`.
    virtual void close() = 0;

    /// @returns true once the user has asked to close the window.
    virtual bool shouldClose() const = 0;

    /// Begins a frame. Valid only between `open` and `close`.
    virtual void beginFrame() = 0;

    /// Ends a frame and presents it. Must pair with `beginFrame`.
    virtual void endFrame() = 0;

    /// @returns The current drawable size in pixels.
    virtual Size size() const = 0;

    /// Captures the cursor: hidden, locked to the window, delivering raw
    /// relative movement.
    ///
    /// @note Required for mouse look. Without it the pointer wanders off the
    ///       window and stops at the screen edge, so turning simply halts.
    ///       Releasing it is equally required - a game that traps the cursor
    ///       with no way out is hostile.
    virtual void setCursorCaptured(bool captured) = 0;

    /// @returns Whether the cursor is currently captured.
    virtual bool isCursorCaptured() const = 0;
};

} // namespace platform
