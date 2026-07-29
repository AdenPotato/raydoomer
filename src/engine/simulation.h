#pragma once

namespace engine {

/// Locked simulation and frame-budget numbers.
///
/// @remarks
/// These are **locked values** in the sense of engine_protocol.md: "Budgets are
/// locked numbers; a change that blows one halts and is flagged rather than
/// merged with a note." They are registered in locked_decisions.md under
/// Architecture and specified in runtime_architecture.md.
///
/// They are deliberately constants in code rather than tunables in data. A
/// designer never adjusts the tick rate: changing it changes what every existing
/// simulation test means. That is the structural-constant exemption in
/// gameplay_protocol.md, and this comment is the "say so when it is not obvious"
/// the rule asks for.
inline constexpr int SIM_TICK_HZ = 60;
inline constexpr double SIM_TICK_SECONDS = 1.0 / static_cast<double>(SIM_TICK_HZ);

/// Presentation target, 144fps. Exceeding this is a flagged regression, not a note.
inline constexpr double FRAME_BUDGET_MS = 6.94;

/// Most simulation ticks one frame may run before the backlog is discarded.
/// Prevents the spiral of death: a slow frame causing more simulation, causing
/// a slower frame.
inline constexpr int MAX_CATCHUP_TICKS = 5;

} // namespace engine
