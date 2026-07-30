#pragma once

#include "platform/violation.h"

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <typeindex>
#include <vector>

namespace engine {

// Lives in platform/ so the renderer seam can use it too; aliased here so
// callers still write engine::ViolationReporter.
using platform::ViolationReporter;
using platform::defaultViolationReporter;

/// Identifies the system that owns a handler.
///
/// @remarks
/// Opaque to the engine, which carries no game nouns (engine_protocol.md). The
/// game assigns whatever meaning it likes; the dispatcher only compares them,
/// to detect a system re-entering itself.
struct SystemId {
    uint32_t value = 0;

    friend bool operator==(const SystemId&, const SystemId&) = default;
};

/// Immediate event dispatch with determinism guardrails.
///
/// @remarks
/// Publishing runs handlers inline, so cause and effect are immediate and
/// followable in a debugger. Immediate dispatch does not satisfy
/// engine_protocol.md's stable-iteration-order rule on its own, so three
/// guardrails are **requirements, not suggestions**:
///
///  1. **Static registration.** Handlers are registered once, before `freeze`,
///     in a fixed and explicit order. There is no runtime subscribe.
///  2. **No structural mutation during dispatch.** A handler may not register
///     another. Collections are mutated by marking, and swept afterwards.
///  3. **Reentrancy guard.** A handler may not re-enter its own system. Event
///     cycles are exactly what the loot systems will produce, and a silent
///     stack overflow is the worst way to find one.
///
/// A change that weakens any of the three is a flagged change.
class EventDispatcher {
public:
    EventDispatcher();
    explicit EventDispatcher(ViolationReporter onViolation);

    /// Registers a handler. Valid only before `freeze`.
    ///
    /// @param owner   The system this handler belongs to, for the reentrancy guard.
    /// @param handler Invoked with each published event of type `Event`.
    /// @note Calling this after `freeze`, or from inside a handler, reports a
    ///       violation and the handler is **rejected** - reporting alone would
    ///       leave the order nondeterministic in a release build.
    template <typename Event>
    void subscribe(SystemId owner, std::function<void(const Event&)> handler) {
        if (dispatching_) {
            report("structural mutation during dispatch: subscribe() called from inside a handler");
            return;
        }
        if (frozen_) {
            report("registration is frozen: subscribe() called after freeze()");
            return;
        }
        handlers_.push_back(Entry{
            std::type_index(typeid(Event)),
            owner,
            [handler = std::move(handler)](const void* event) {
                handler(*static_cast<const Event*>(event));
            },
        });
    }

    /// Closes registration. Call once at startup, after every subscribe.
    void freeze();

    /// Publishes an event, running its handlers inline in registration order.
    ///
    /// @note A handler whose system is already on the dispatch stack is skipped
    ///       and reported. Skipping rather than recursing is what turns a cycle
    ///       into a diagnosable violation instead of a stack overflow.
    template <typename Event>
    void publish(const Event& event) {
        const std::type_index type{ typeid(Event) };

        // Iterating by index over a vector that cannot be structurally mutated
        // mid-dispatch: order is the registration order, always.
        const bool wasDispatching = dispatching_;
        dispatching_ = true;

        for (size_t i = 0; i < handlers_.size(); ++i) {
            const Entry& entry = handlers_[i];
            if (entry.type != type) {
                continue;
            }
            if (isSystemActive(entry.owner)) {
                report("reentrancy: system " + std::to_string(entry.owner.value) +
                       " re-entered while already dispatching");
                continue;
            }
            activeSystems_.push_back(entry.owner);
            entry.invoke(&event);
            activeSystems_.pop_back();
        }

        dispatching_ = wasDispatching;
    }

private:
    struct Entry {
        std::type_index type;
        SystemId owner;
        std::function<void(const void*)> invoke;
    };

    bool isSystemActive(SystemId id) const;
    void report(std::string_view message) const;

    std::vector<Entry> handlers_;
    std::vector<SystemId> activeSystems_;
    ViolationReporter onViolation_;
    bool frozen_ = false;
    bool dispatching_ = false;
};

} // namespace engine
