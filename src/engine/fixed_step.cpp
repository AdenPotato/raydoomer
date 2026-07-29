#include "engine/fixed_step.h"

#include <cmath>

namespace engine {

FixedStepAccumulator::FixedStepAccumulator(double tickSeconds, int maxCatchupTicks)
    : tickSeconds_(tickSeconds), maxCatchupTicks_(maxCatchupTicks) {}

StepResult FixedStepAccumulator::advance(double frameDeltaSeconds) {
    // A steady clock should never hand us these, but a guard here is cheaper
    // than a rewound simulation and much easier to reason about than the bug it
    // would otherwise cause.
    if (std::isfinite(frameDeltaSeconds) && frameDeltaSeconds > 0.0) {
        accumulator_ += frameDeltaSeconds;
    }

    int ticks = 0;
    while (accumulator_ >= tickSeconds_ && ticks < maxCatchupTicks_) {
        accumulator_ -= tickSeconds_;
        ++ticks;
    }

    // Capped: drop the backlog instead of carrying it. See the note on advance().
    if (accumulator_ >= tickSeconds_) {
        accumulator_ = std::fmod(accumulator_, tickSeconds_);
    }

    return StepResult{ ticks, alpha() };
}

double FixedStepAccumulator::alpha() const {
    return accumulator_ / tickSeconds_;
}

} // namespace engine
