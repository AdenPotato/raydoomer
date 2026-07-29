#include "engine/event_dispatch.h"

#include <algorithm>

namespace engine {

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
