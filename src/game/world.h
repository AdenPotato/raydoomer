#pragma once

namespace game {

/// The root container for simulation state.
///
/// @remarks
/// **This is a stub.** The real data model - entities, systems, ownership, and
/// the explicit tick order - is ADE-11 and ADE-12 onward. Building any of it
/// here would be speculative generality (gameplay_protocol.md, Enforcement
/// Rule 5).
///
/// What exists is exactly the contract the frame loop needs: something to tick,
/// and enough observable state to prove it was ticked correctly. When the real
/// world arrives, `tick` grows the injected context - `tick(dt, Rng&,
/// EventQueue&)` - and everything else here is replaced.
///
/// It reads no clock and touches no device, so it steps headless.
class World {
public:
    /// Advances the simulation by one fixed tick.
    /// @param dt Length of the tick in simulated seconds. Always the fixed step;
    ///           never a variable frame delta.
    void tick(float dt);

    /// @returns How many ticks have been run.
    int tickCount() const { return tickCount_; }

    /// @returns Simulated seconds elapsed. Not wall-clock time.
    float elapsedSeconds() const { return elapsedSeconds_; }

private:
    int tickCount_ = 0;
    float elapsedSeconds_ = 0.0f;
};

} // namespace game
