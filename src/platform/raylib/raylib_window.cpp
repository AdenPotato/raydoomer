#include "platform/raylib/raylib_window.h"

#include <raylib.h>

namespace platform {

RaylibWindow::~RaylibWindow() {
    // Every acquire has a matching release, including on the path where the
    // caller forgot. Destruction is the last line of defence, not the plan.
    close();
}

bool RaylibWindow::open(int width, int height, const char* title) {
    if (isOpen_) {
        return true;
    }
    InitWindow(width, height, title);
    isOpen_ = IsWindowReady();
    if (!isOpen_) {
        // Leave nothing half-acquired behind a failed open.
        CloseWindow();
    }
    return isOpen_;
}

void RaylibWindow::close() {
    // Safe to call more than once, and safe after a failed open.
    if (!isOpen_) {
        return;
    }
    // Release the cursor before the window goes: leaving it captured after the
    // window is gone strands the pointer.
    if (cursorCaptured_) {
        EnableCursor();
        cursorCaptured_ = false;
    }
    CloseWindow();
    isOpen_ = false;
}

bool RaylibWindow::shouldClose() const {
    return !isOpen_ || WindowShouldClose();
}

void RaylibWindow::beginFrame() {
    BeginDrawing();
    // raylib's BeginDrawing does not clear, so without this the window shows an
    // uninitialised backbuffer. Clearing here is a platform-level default, not a
    // rendering decision: when the renderer lands it owns what fills the frame,
    // and this drops to whatever it needs.
    ClearBackground(Color{ 24, 24, 32, 255 });
}

void RaylibWindow::endFrame() {
    EndDrawing();
}

void RaylibWindow::setCursorCaptured(bool captured) {
    if (!isOpen_ || captured == cursorCaptured_) {
        return;
    }
    if (captured) {
        DisableCursor(); // hides, locks to window, gives relative movement
    } else {
        EnableCursor();
    }
    cursorCaptured_ = captured;
}

bool RaylibWindow::isCursorCaptured() const {
    return cursorCaptured_;
}

Size RaylibWindow::size() const {
    if (!isOpen_) {
        return Size{};
    }
    return Size{ GetScreenWidth(), GetScreenHeight() };
}

} // namespace platform
