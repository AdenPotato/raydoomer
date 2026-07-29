#pragma once

namespace engine {

/// What one rendered frame owes the simulation.
struct StepResult {
    /// How many fixed ticks to run before drawing. May be zero.
    int ticks = 0;

    /// Progress toward the next tick, in [0, 1). Presentation blends the two
    /// most recent simulation states by this value. Never reaches 1: that would
    /// draw a state that has not been simulated yet.
    double alpha = 0.0;
};

/// Translates variable frame time into a whole number of fixed simulation ticks.
///
/// @remarks
/// Simulation advances on a fixed step; presentation runs on a variable one
/// (engine_protocol.md). This type owns that translation and nothing else - no
/// clock, no window, no game concepts - which is what makes it pure and
/// deterministic.
///
/// The remainder carries between frames, so no simulated time is lost. Long
/// frames are capped rather than banked: see @ref advance.
class FixedStepAccumulator {
public:
    /// @param tickSeconds     Length of one simulation tick, in seconds.
    /// @param maxCatchupTicks Most ticks a single frame may run.
    FixedStepAccumulator(double tickSeconds, int maxCatchupTicks);

    /// Consumes one frame's elapsed time.
    ///
    /// @param frameDeltaSeconds Real seconds since the previous frame. Values
    ///        that are negative or not finite are ignored rather than allowed to
    ///        rewind the simulation.
    /// @returns The ticks to run and the interpolation alpha to draw with.
    ///
    /// @note When the frame is long enough to exceed `maxCatchupTicks`, the
    ///       excess is **discarded, not banked**. Banking it would leave the
    ///       loop permanently in debt, running the maximum every frame - the
    ///       spiral of death. The trade is deliberate: after a hitch the game
    ///       continues rather than fast-forwarding.
    StepResult advance(double frameDeltaSeconds);

    /// Current progress toward the next tick, in [0, 1).
    double alpha() const;

private:
    double tickSeconds_;
    int maxCatchupTicks_;
    double accumulator_ = 0.0;
};

} // namespace engine
