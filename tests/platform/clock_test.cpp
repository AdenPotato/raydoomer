// The injected clock - seam 1 of the four (game_test_protocol.md).
//
// Nothing in the game may read the wall clock. Time arrives as a parameter, so
// a test can advance it exactly. These tests pin both halves: the real clock
// behaves sanely, and the manual clock advances precisely as told.

#include <gtest/gtest.h>

#include "platform/clock.h"
#include "support/manual_clock.h"

namespace {

TEST(ManualClock, StartsAtZero) {
    support::ManualClock clock;
    EXPECT_DOUBLE_EQ(clock.nowSeconds(), 0.0);
}

TEST(ManualClock, AdvancesExactlyAsTold) {
    support::ManualClock clock;

    clock.advance(0.5);
    EXPECT_DOUBLE_EQ(clock.nowSeconds(), 0.5);

    clock.advance(0.25);
    EXPECT_DOUBLE_EQ(clock.nowSeconds(), 0.75);
}

TEST(ManualClock, AdvancingByOneTickSixtyTimesGivesExactlyOneSecond) {
    // The property the fixed-step loop depends on. Accumulated floating point
    // drift here would make every time-dependent test subtly wrong, so assert
    // it directly rather than assuming it.
    support::ManualClock clock;
    constexpr double kTick = 1.0 / 60.0;

    for (int i = 0; i < 60; ++i) {
        clock.advance(kTick);
    }

    EXPECT_NEAR(clock.nowSeconds(), 1.0, 1e-9);
}

TEST(ManualClock, NeverGoesBackwards) {
    support::ManualClock clock;
    clock.advance(1.0);
    const double before = clock.nowSeconds();
    clock.advance(0.0);
    EXPECT_GE(clock.nowSeconds(), before);
}

TEST(SystemClock, IsMonotonic) {
    // A steady clock must never step backwards, even if the wall clock is
    // adjusted underneath it. This is why it is a steady_clock and not a
    // system_clock: an NTP correction mid-frame would otherwise produce a
    // negative delta and a physics explosion.
    platform::SystemClock clock;

    double previous = clock.nowSeconds();
    for (int i = 0; i < 1000; ++i) {
        const double now = clock.nowSeconds();
        EXPECT_GE(now, previous);
        previous = now;
    }
}

TEST(SystemClock, IsUsableThroughTheInterface) {
    // The whole point of the seam: callers depend on Clock, never on the
    // concrete type.
    platform::SystemClock concrete;
    platform::Clock& clock = concrete;
    EXPECT_GE(clock.nowSeconds(), 0.0);
}

} // namespace
