#pragma once

#include <chrono>

namespace platform {

/// A monotonic time source.
///
/// @remarks
/// The engine owns the clock and hands it out; subsystem logic receives time as
/// a parameter and never reads the wall clock itself. This is a hard
/// requirement, not a testing convenience - see the four seams in
/// game_test_protocol.md.
///
/// Units are **seconds** since an unspecified epoch. Only differences between
/// readings are meaningful; the absolute value is not.
class Clock {
public:
    virtual ~Clock() = default;

    /// @returns Monotonically non-decreasing seconds. Never steps backwards.
    virtual double nowSeconds() const = 0;
};

/// The real clock, backed by `std::chrono::steady_clock`.
///
/// @remarks
/// Deliberately a steady clock rather than a system clock: a wall-clock
/// adjustment (an NTP correction, a daylight-saving change) would otherwise
/// produce a negative frame delta and a physics explosion.
class SystemClock final : public Clock {
public:
    SystemClock();

    double nowSeconds() const override;

private:
    std::chrono::steady_clock::time_point start_;
};

} // namespace platform
