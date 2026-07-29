#pragma once

#include "platform/clock.h"

namespace support {

/// A clock a test advances by hand.
///
/// @remarks
/// Seam 1 of the four (game_test_protocol.md): "Nothing reads the wall clock.
/// Time arrives as a parameter, so a test can advance it exactly."
///
/// This is the type that makes every time-dependent test possible. A test steps
/// the simulation by an explicit number of ticks; it never sleeps. A test
/// containing a sleep is wrong, slow, and flaky.
class ManualClock final : public platform::Clock {
public:
    double nowSeconds() const override { return seconds_; }

    /// Advances time by exactly `seconds`. Negative values are rejected: time
    /// running backwards is a determinism bug, not a testing technique.
    void advance(double seconds) {
        if (seconds > 0.0) {
            seconds_ += seconds;
        }
    }

private:
    double seconds_ = 0.0;
};

} // namespace support
