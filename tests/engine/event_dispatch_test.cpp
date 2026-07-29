// Immediate event dispatch, and the three guardrails that make it deterministic.
//
// Immediate dispatch was chosen over a deferred queue: publishing runs handlers
// inline, so cause and effect are immediate and followable in a debugger. On its
// own that does NOT satisfy engine_protocol.md's stable-iteration-order rule, so
// three guardrails are mandatory rather than optional:
//
//   1. static registration  - handlers registered once, in a fixed order
//   2. no structural mutation during dispatch
//   3. a reentrancy guard   - a handler may not re-enter its own system
//
// These tests are what make those guardrails real. Weakening any of them later
// should fail here loudly.

#include <gtest/gtest.h>

#include "engine/event_dispatch.h"

#include <string>
#include <vector>

namespace {

// Event types are the game's to define; the dispatcher only needs them to be
// distinct types. These stand in for the real ones.
struct Damaged {
    int amount = 0;
};

struct Died {
    int entity = 0;
};

// Systems are opaque ids to the engine - it carries no game nouns
// (engine_protocol.md), so the game assigns whatever meaning it likes.
constexpr engine::SystemId kCombat{ 1 };
constexpr engine::SystemId kLoot{ 2 };
constexpr engine::SystemId kAudio{ 3 };

// Collects guardrail violations so a test can assert on them instead of the
// process aborting. In a development build the default reporter is loud.
struct ViolationLog {
    std::vector<std::string> messages;

    engine::ViolationReporter reporter() {
        return [this](std::string_view message) { messages.emplace_back(message); };
    }
};

TEST(EventDispatch, DeliversAnEventToItsHandler) {
    engine::EventDispatcher dispatcher;
    int received = 0;

    dispatcher.subscribe<Damaged>(kCombat, [&](const Damaged& e) { received = e.amount; });
    dispatcher.freeze();

    dispatcher.publish(Damaged{ 7 });

    EXPECT_EQ(received, 7);
}

TEST(EventDispatch, DispatchesImmediatelyRatherThanDeferring) {
    // The defining property of the chosen model: the handler has already run by
    // the time publish() returns. A deferred queue would fail this.
    engine::EventDispatcher dispatcher;
    bool ranDuringPublish = false;

    dispatcher.subscribe<Damaged>(kCombat, [&](const Damaged&) { ranDuringPublish = true; });
    dispatcher.freeze();

    dispatcher.publish(Damaged{ 1 });
    EXPECT_TRUE(ranDuringPublish);
}

TEST(EventDispatch, DeliversOnlyToHandlersOfThatEventType) {
    engine::EventDispatcher dispatcher;
    int damaged = 0;
    int died = 0;

    dispatcher.subscribe<Damaged>(kCombat, [&](const Damaged&) { ++damaged; });
    dispatcher.subscribe<Died>(kLoot, [&](const Died&) { ++died; });
    dispatcher.freeze();

    dispatcher.publish(Damaged{ 1 });

    EXPECT_EQ(damaged, 1);
    EXPECT_EQ(died, 0);
}

TEST(EventDispatch, PublishingAnEventWithNoHandlersIsHarmless) {
    engine::EventDispatcher dispatcher;
    dispatcher.freeze();
    EXPECT_NO_THROW(dispatcher.publish(Died{ 42 }));
}

// --- Guardrail 1: static registration, fixed order -------------------------

TEST(EventDispatch, HandlersRunInRegistrationOrder) {
    // The order is the registration order, always. This is what replaces the
    // "subscription order" nondeterminism that made immediate dispatch a risk.
    engine::EventDispatcher dispatcher;
    std::vector<int> order;

    dispatcher.subscribe<Damaged>(kCombat, [&](const Damaged&) { order.push_back(1); });
    dispatcher.subscribe<Damaged>(kLoot, [&](const Damaged&) { order.push_back(2); });
    dispatcher.subscribe<Damaged>(kAudio, [&](const Damaged&) { order.push_back(3); });
    dispatcher.freeze();

    dispatcher.publish(Damaged{ 1 });

    EXPECT_EQ(order, (std::vector<int>{ 1, 2, 3 }));
}

TEST(EventDispatch, TwoIdenticalRunsProduceIdenticalCallOrder) {
    // The determinism assertion engine_protocol.md's stable-iteration rule
    // demands. Same registrations, same publishes, same order - every time.
    auto run = [] {
        engine::EventDispatcher dispatcher;
        std::vector<int> order;

        dispatcher.subscribe<Damaged>(kCombat, [&](const Damaged& e) { order.push_back(10 + e.amount); });
        dispatcher.subscribe<Damaged>(kLoot, [&](const Damaged& e) { order.push_back(20 + e.amount); });
        dispatcher.subscribe<Died>(kAudio, [&](const Died& e) { order.push_back(30 + e.entity); });
        dispatcher.freeze();

        dispatcher.publish(Damaged{ 1 });
        dispatcher.publish(Died{ 2 });
        dispatcher.publish(Damaged{ 3 });
        return order;
    };

    EXPECT_EQ(run(), run());
}

TEST(EventDispatch, SubscribingAfterFreezeIsAViolation) {
    // There is no runtime subscribe. Registration happens once, at startup, in
    // one readable place - otherwise the real order is scattered across call
    // sites and stops being reviewable.
    ViolationLog log;
    engine::EventDispatcher dispatcher{ log.reporter() };

    dispatcher.subscribe<Damaged>(kCombat, [](const Damaged&) {});
    dispatcher.freeze();

    dispatcher.subscribe<Damaged>(kLoot, [](const Damaged&) {});

    ASSERT_EQ(log.messages.size(), 1u);
    EXPECT_NE(log.messages[0].find("frozen"), std::string::npos);
}

TEST(EventDispatch, AHandlerRegisteredAfterFreezeNeverRuns) {
    // Reporting the violation is not enough; the late handler must also be
    // rejected, or the order is still nondeterministic in a release build.
    ViolationLog log;
    engine::EventDispatcher dispatcher{ log.reporter() };
    bool lateRan = false;

    dispatcher.freeze();
    dispatcher.subscribe<Damaged>(kLoot, [&](const Damaged&) { lateRan = true; });
    dispatcher.publish(Damaged{ 1 });

    EXPECT_FALSE(lateRan);
}

// --- Guardrail 2: no structural mutation during dispatch -------------------

TEST(EventDispatch, SubscribingFromInsideAHandlerIsAViolation) {
    // Structural mutation mid-dispatch is the iterator-invalidation crash this
    // guardrail exists to prevent.
    ViolationLog log;
    engine::EventDispatcher dispatcher{ log.reporter() };

    dispatcher.subscribe<Damaged>(kCombat, [&](const Damaged&) {
        dispatcher.subscribe<Died>(kLoot, [](const Died&) {});
    });
    dispatcher.freeze();

    dispatcher.publish(Damaged{ 1 });

    ASSERT_FALSE(log.messages.empty());
}

TEST(EventDispatch, MarkDeadThenSweepIsSafeDuringDispatch) {
    // The pattern that replaces structural mutation: a handler marks, and the
    // owner sweeps after dispatch completes. Demonstrated here on a plain
    // collection; entity-level sweeping arrives with the real World.
    engine::EventDispatcher dispatcher;
    std::vector<std::pair<int, bool>> items{ { 1, false }, { 2, false }, { 3, false } };

    dispatcher.subscribe<Died>(kCombat, [&](const Died& e) {
        for (auto& [id, dead] : items) {
            if (id == e.entity) {
                dead = true; // mark, never erase
            }
        }
    });
    dispatcher.freeze();

    dispatcher.publish(Died{ 2 });

    // Sweep happens after dispatch, never inside it.
    std::erase_if(items, [](const auto& item) { return item.second; });

    ASSERT_EQ(items.size(), 2u);
    EXPECT_EQ(items[0].first, 1);
    EXPECT_EQ(items[1].first, 3);
}

// --- Guardrail 3: reentrancy ------------------------------------------------

TEST(EventDispatch, AHandlerReenteringItsOwnSystemIsAViolation) {
    // died -> drop -> pickup -> died is exactly the cycle the loot systems will
    // produce. Without this guard it is a silent stack overflow with no
    // diagnostic.
    ViolationLog log;
    engine::EventDispatcher dispatcher{ log.reporter() };

    dispatcher.subscribe<Damaged>(kCombat, [&](const Damaged&) {
        dispatcher.publish(Died{ 1 });
    });
    dispatcher.subscribe<Died>(kCombat, [&](const Died&) {
        // Same system re-entered: this is the violation.
    });
    dispatcher.freeze();

    dispatcher.publish(Damaged{ 1 });

    ASSERT_FALSE(log.messages.empty());
    EXPECT_NE(log.messages[0].find("reentran"), std::string::npos);
}

TEST(EventDispatch, ADifferentSystemRespondingIsNotAViolation) {
    // The guard must not fire on ordinary cross-system flow, or it is useless.
    // combat -> died -> loot reacts is exactly what the design intends.
    ViolationLog log;
    engine::EventDispatcher dispatcher{ log.reporter() };
    bool lootRan = false;

    dispatcher.subscribe<Damaged>(kCombat, [&](const Damaged&) {
        dispatcher.publish(Died{ 1 });
    });
    dispatcher.subscribe<Died>(kLoot, [&](const Died&) { lootRan = true; });
    dispatcher.freeze();

    dispatcher.publish(Damaged{ 1 });

    EXPECT_TRUE(lootRan);
    EXPECT_TRUE(log.messages.empty()) << "the guard fired on legitimate cross-system flow";
}

TEST(EventDispatch, ACycleIsStoppedRatherThanOverflowingTheStack) {
    // The failure mode this exists to prevent. If the guard only reported and
    // still recursed, this test would crash the runner rather than fail.
    ViolationLog log;
    engine::EventDispatcher dispatcher{ log.reporter() };

    dispatcher.subscribe<Damaged>(kCombat, [&](const Damaged& e) {
        dispatcher.publish(Damaged{ e.amount + 1 });
    });
    dispatcher.freeze();

    dispatcher.publish(Damaged{ 1 });

    ASSERT_FALSE(log.messages.empty());
}

#ifndef NDEBUG
TEST(EventDispatchDeathTest, TheDefaultReporterIsLoudInADevelopmentBuild) {
    // Every other test injects a reporter so it can assert instead of dying.
    // This one exercises the DEFAULT path, because "it aborts in development
    // builds" is a claim in the docstring and an unverified claim is a guess
    // (engine_protocol.md: assert on programmer error).
    EXPECT_DEATH(
        {
            engine::EventDispatcher dispatcher; // default reporter
            dispatcher.freeze();
            dispatcher.subscribe<Damaged>(kCombat, [](const Damaged&) {});
        },
        "GUARDRAIL VIOLATION");
}
#endif

TEST(EventDispatch, TheGuardResetsBetweenTopLevelPublishes) {
    // A system that legitimately handles the same event twice in a tick must not
    // be flagged. The guard is about re-entry, not about frequency.
    ViolationLog log;
    engine::EventDispatcher dispatcher{ log.reporter() };
    int runs = 0;

    dispatcher.subscribe<Damaged>(kCombat, [&](const Damaged&) { ++runs; });
    dispatcher.freeze();

    dispatcher.publish(Damaged{ 1 });
    dispatcher.publish(Damaged{ 2 });

    EXPECT_EQ(runs, 2);
    EXPECT_TRUE(log.messages.empty());
}

} // namespace
