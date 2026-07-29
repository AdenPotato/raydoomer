// The locked numbers, asserted against the canon.
//
// engine_protocol.md: "Budgets are locked numbers; a change that blows one halts
// and is flagged rather than merged with a note." A constant can be edited in a
// second and reviewed in none, so these assertions exist to make that edit
// visible - the test fails and someone has to explain why.
//
// If one of these legitimately changes, the unlock process runs first
// (locked_decisions.md), and this test is updated as part of it.

#include <gtest/gtest.h>

#include "engine/simulation.h"

namespace {

TEST(SimulationConstants, MatchTheLockedDecisions) {
    EXPECT_EQ(engine::SIM_TICK_HZ, 60);
    EXPECT_DOUBLE_EQ(engine::SIM_TICK_SECONDS, 1.0 / 60.0);
    EXPECT_DOUBLE_EQ(engine::FRAME_BUDGET_MS, 6.94);
    EXPECT_EQ(engine::MAX_CATCHUP_TICKS, 5);
}

TEST(SimulationConstants, TickSecondsIsConsistentWithTickHz) {
    // Guards the pair drifting apart: editing one without the other would give
    // a simulation that ticks at one rate and reports another.
    EXPECT_NEAR(engine::SIM_TICK_SECONDS * engine::SIM_TICK_HZ, 1.0, 1e-12);
}

TEST(SimulationConstants, FrameBudgetIsShorterThanASimulationTick) {
    // Presentation is meant to run ahead of simulation; that is why
    // interpolation exists. If the frame budget ever exceeded the tick, the
    // decoupling would be pointless.
    const double tickMs = engine::SIM_TICK_SECONDS * 1000.0;
    EXPECT_LT(engine::FRAME_BUDGET_MS, tickMs);
}

} // namespace
