#include "platform/raylib/raylib_input.h"

#include <raylib.h>

namespace platform {
namespace {

/// The single place a logical action becomes a physical key.
int toRaylibKey(Key key) {
    switch (key) {
        case Key::Forward:  return KEY_W;
        case Key::Back:     return KEY_S;
        case Key::Left:     return KEY_A;
        case Key::Right:    return KEY_D;
        case Key::Jump:     return KEY_SPACE;
        case Key::Fire:     return KEY_LEFT_CONTROL;
        case Key::Interact: return KEY_E;

        // Function keys, so a debug control can never collide with a game action.
        case Key::DebugView:      return KEY_F1;
        case Key::DebugCycleMode: return KEY_F2;
        case Key::DebugOverlay:   return KEY_F3;
    }
    return KEY_NULL;
}

} // namespace

void RaylibInput::poll() {
    // Sampled once, here. Everything read afterwards reflects this instant, so
    // two systems in the same tick cannot observe different input.
    const Vector2 raw = GetMouseDelta();
    delta_ = MouseDelta{ raw.x, raw.y };
}

bool RaylibInput::isKeyDown(Key key) const {
    return IsKeyDown(toRaylibKey(key));
}

MouseDelta RaylibInput::mouseDelta() const {
    return delta_;
}

} // namespace platform
