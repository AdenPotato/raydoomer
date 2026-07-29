#include "platform/clock.h"

namespace platform {

SystemClock::SystemClock()
    : start_(std::chrono::steady_clock::now()) {}

double SystemClock::nowSeconds() const {
    const auto elapsed = std::chrono::steady_clock::now() - start_;
    return std::chrono::duration<double>(elapsed).count();
}

} // namespace platform
