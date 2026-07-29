#pragma once

#include "platform/input.h"

namespace platform {

/// raylib-backed input.
///
/// @remarks
/// Owns the mapping from logical action to physical key. That mapping lives
/// here and nowhere else, so rebinding never reaches gameplay.
class RaylibInput final : public Input {
public:
    void poll() override;
    bool isKeyDown(Key key) const override;
    MouseDelta mouseDelta() const override;

private:
    MouseDelta delta_{};
};

} // namespace platform
