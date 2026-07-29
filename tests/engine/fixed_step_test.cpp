// The fixed-step accumulator: the heart of the frame loop.
//
// Simulation advances on a fixed step; presentation runs on a variable one
// (engine_protocol.md). This type owns that translation and nothing else, which
// is what makes it pure, deterministic, and testable with no window and no clock.
//
// Almost every rule in gameplay_protocol.md about frame-rate independence is
// only true if this type is correct, so the edge cases get asserted directly.

#include <gtest/gtest.h>

#include "engine/fixed_step.h"

namespace {

constexpr double kTick = 1.0 / 60.0;
constexpr int kMaxCatchup = 5;

engine::FixedStepAccumulator makeAccumulator() {
    return engine::FixedStepAccumulator{ kTick, kMaxCatchup };
}

TEST(FixedStep, AFrameOfExactlyOneTickRunsOneTick) {
    auto acc = makeAccumulator();
    EXPECT_EQ(acc.advance(kTick).ticks, 1);
}

TEST(FixedStep, SixtyFramesRunSixtyTicks) {
    // One simulated second at the locked rate. This is the property every
    // time-dependent test downstream relies on.
    auto acc = makeAccumulator();

    int total = 0;
    for (int i = 0; i < 60; ++i) {
        total += acc.advance(kTick).ticks;
    }

    EXPECT_EQ(total, 60);
}

TEST(FixedStep, AFrameShorterThanOneTickRunsNoTicks) {
    // At 144fps against a 60Hz simulation, most frames run no tick at all.
    // Getting this wrong is how a game becomes frame-rate dependent.
    auto acc = makeAccumulator();
    const double shortFrame = 1.0 / 144.0;

    EXPECT_EQ(acc.advance(shortFrame).ticks, 0);
}

TEST(FixedStep, ShortFramesEventuallyAccumulateIntoATick) {
    // 1/144 is 0.4 of a tick, so the third short frame must produce one.
    auto acc = makeAccumulator();
    const double shortFrame = 1.0 / 144.0;

    EXPECT_EQ(acc.advance(shortFrame).ticks, 0);
    EXPECT_EQ(acc.advance(shortFrame).ticks, 0);
    EXPECT_EQ(acc.advance(shortFrame).ticks, 1);
}

TEST(FixedStep, TimeIsNotLostBetweenFrames) {
    // The remainder carries. If it were dropped, the simulation would run slow
    // by a fraction of a tick every frame - a drift too small to notice and
    // impossible to debug later.
    auto acc = makeAccumulator();
    const double oneAndAHalfTicks = kTick * 1.5;

    EXPECT_EQ(acc.advance(oneAndAHalfTicks).ticks, 1);
    // Half a tick is banked; another half completes the second tick.
    EXPECT_EQ(acc.advance(kTick * 0.5).ticks, 1);
}

TEST(FixedStep, ALongFrameIsCappedByMaxCatchupTicks) {
    // A 100ms hitch is 6 ticks' worth, but the cap is 5. Without the cap, a
    // slow frame causes more simulation, which causes a slower frame: the
    // spiral of death.
    auto acc = makeAccumulator();

    EXPECT_EQ(acc.advance(0.100).ticks, kMaxCatchup);
}

TEST(FixedStep, CappingDiscardsTheExcessRatherThanBankingIt) {
    // The excess must NOT be carried, or the loop stays permanently in debt and
    // every subsequent frame runs the maximum. Deliberately trading simulated
    // time for recovery: after a hitch the game continues, it does not
    // fast-forward.
    auto acc = makeAccumulator();

    acc.advance(1.0); // ~60 ticks' worth, capped to 5
    // The next ordinary frame must behave ordinarily.
    EXPECT_EQ(acc.advance(kTick).ticks, 1);
}

TEST(FixedStep, AlphaIsAlwaysInTheUnitIntervalExcludingOne) {
    // Presentation interpolates by alpha. A value of exactly 1 would draw the
    // next state before it has been simulated.
    auto acc = makeAccumulator();

    for (int i = 0; i < 500; ++i) {
        const auto result = acc.advance(kTick * 0.37); // deliberately not a divisor
        EXPECT_GE(result.alpha, 0.0);
        EXPECT_LT(result.alpha, 1.0);
    }
}

TEST(FixedStep, AlphaIsZeroWhenAFrameLandsExactlyOnATickBoundary) {
    auto acc = makeAccumulator();
    EXPECT_NEAR(acc.advance(kTick).alpha, 0.0, 1e-12);
}

TEST(FixedStep, AlphaReflectsPartialProgressTowardTheNextTick) {
    auto acc = makeAccumulator();
    const auto result = acc.advance(kTick * 0.25);

    EXPECT_EQ(result.ticks, 0);
    EXPECT_NEAR(result.alpha, 0.25, 1e-12);
}

TEST(FixedStep, AZeroLengthFrameRunsNoTicksAndDoesNotMoveAlpha) {
    auto acc = makeAccumulator();
    acc.advance(kTick * 0.5);
    const double before = acc.alpha();

    const auto result = acc.advance(0.0);

    EXPECT_EQ(result.ticks, 0);
    EXPECT_DOUBLE_EQ(result.alpha, before);
}

TEST(FixedStep, ANegativeFrameDeltaIsIgnoredRatherThanRewindingTime) {
    // Should be impossible with a steady clock, but a negative delta must never
    // rewind the simulation. Assert the guard rather than trust the clock.
    auto acc = makeAccumulator();
    acc.advance(kTick * 0.5);
    const double before = acc.alpha();

    const auto result = acc.advance(-1.0);

    EXPECT_EQ(result.ticks, 0);
    EXPECT_DOUBLE_EQ(result.alpha, before);
}

TEST(FixedStep, IsDeterministicAcrossIdenticalRuns) {
    // Same inputs, same outputs. A divergence here would make every simulation
    // test unreproducible.
    auto first = makeAccumulator();
    auto second = makeAccumulator();

    const double deltas[] = { 0.004, 0.020, 0.100, 0.001, 0.0167, 0.033 };
    for (double delta : deltas) {
        const auto a = first.advance(delta);
        const auto b = second.advance(delta);
        EXPECT_EQ(a.ticks, b.ticks);
        EXPECT_DOUBLE_EQ(a.alpha, b.alpha);
    }
}

} // namespace
