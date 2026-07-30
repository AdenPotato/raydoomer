#pragma once

#include "platform/window.h"

namespace platform {

/// raylib-backed window.
///
/// @remarks
/// This is the only place a window is actually created. It exists solely in the
/// Windows build; the headless test configuration never compiles it, which is
/// what guarantees a test cannot accidentally open one.
class RaylibWindow final : public Window {
public:
    ~RaylibWindow() override;

    bool open(int width, int height, const char* title) override;
    void close() override;
    bool shouldClose() const override;
    void beginFrame() override;
    void endFrame() override;
    Size size() const override;
    void setCursorCaptured(bool captured) override;
    bool isCursorCaptured() const override;

private:
    bool isOpen_ = false;
    bool cursorCaptured_ = false;
};

} // namespace platform
