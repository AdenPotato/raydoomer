#pragma once

#include <functional>
#include <string_view>

namespace platform {

/// Called when an engine invariant is violated.
///
/// @remarks
/// Injected rather than global, so two tests in the same process cannot
/// contaminate each other (seam 4, game_test_protocol.md).
///
/// The default reports to stderr and aborts in a development build: a violated
/// invariant is programmer error and should be loud, while an operational error
/// is returned as a typed result instead (engine_protocol.md).
using ViolationReporter = std::function<void(std::string_view message)>;

/// The default reporter: loud in development, still reported in release.
ViolationReporter defaultViolationReporter();

} // namespace platform
