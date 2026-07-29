#include "engine/violation.h"

#include <cstdio>
#include <cstdlib>

namespace engine {

ViolationReporter defaultViolationReporter() {
    return [](std::string_view message) {
        std::fprintf(stderr, "[engine] GUARDRAIL VIOLATION: %.*s\n",
                     static_cast<int>(message.size()), message.data());
#ifndef NDEBUG
        // A violated invariant is programmer error, and should be loud in a
        // development build rather than discovered later as a subtle
        // nondeterminism (engine_protocol.md).
        std::abort();
#endif
    };
}

} // namespace engine
