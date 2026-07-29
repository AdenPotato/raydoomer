#include "engine/event_dispatch.h"

#include <algorithm>
#include <cstdio>
#include <cstdlib>

namespace engine {

ViolationReporter defaultViolationReporter() {
    return [](std::string_view message) {
        std::fprintf(stderr, "[event-dispatch] GUARDRAIL VIOLATION: %.*s\n",
                     static_cast<int>(message.size()), message.data());
#ifndef NDEBUG
        // A violated invariant is programmer error, and should be loud in a
        // development build rather than discovered later as a subtle
        // nondeterminism (engine_protocol.md).
        std::abort();
#endif
    };
}

EventDispatcher::EventDispatcher()
    : onViolation_(defaultViolationReporter()) {}

EventDispatcher::EventDispatcher(ViolationReporter onViolation)
    : onViolation_(std::move(onViolation)) {}

void EventDispatcher::freeze() {
    frozen_ = true;
    // Registration is closed, so the handler list will never grow again. Release
    // the surplus capacity: nothing allocates in the hot path afterwards
    // (engine_protocol.md).
    handlers_.shrink_to_fit();
    // The reentrancy stack can never exceed one entry per registered handler.
    // Reserving here means publish() never allocates.
    activeSystems_.reserve(handlers_.size());
}

bool EventDispatcher::isSystemActive(SystemId id) const {
    return std::find(activeSystems_.begin(), activeSystems_.end(), id) != activeSystems_.end();
}

void EventDispatcher::report(std::string_view message) const {
    if (onViolation_) {
        onViolation_(message);
    }
}

} // namespace engine
