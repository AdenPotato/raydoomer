// The seeded, injected RNG - seam 2 of the four (game_test_protocol.md).
//
// "Nothing reaches for a global random source. A test seeds it and gets the same
// run every time." Loot rolls, weapon spread, and enemy decisions all run
// through this, so its reproducibility is the reproducibility of the whole game.
//
// The saved RNG seed (runtime_architecture.md) means a save file is only
// meaningful if the algorithm itself is stable, which is why one test pins exact
// output values rather than only properties.

#include <gtest/gtest.h>

#include "engine/rng.h"

#include <set>
#include <vector>

namespace {

std::vector<uint32_t> firstN(engine::Rng& rng, int n) {
    std::vector<uint32_t> values;
    values.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        values.push_back(rng.nextUint32());
    }
    return values;
}

TEST(Rng, TheSameSeedProducesTheSameSequence) {
    engine::Rng first{ 12345 };
    engine::Rng second{ 12345 };

    EXPECT_EQ(firstN(first, 50), firstN(second, 50));
}

TEST(Rng, DifferentSeedsProduceDifferentSequences) {
    engine::Rng first{ 1 };
    engine::Rng second{ 2 };

    EXPECT_NE(firstN(first, 50), firstN(second, 50));
}

TEST(Rng, ReseedingRestartsTheSequence) {
    // Loading a save restores the seed and must resume the same stream.
    engine::Rng rng{ 999 };
    const auto before = firstN(rng, 20);

    rng.seed(999);
    const auto after = firstN(rng, 20);

    EXPECT_EQ(before, after);
}

TEST(Rng, ProducesKnownValuesForAKnownSeed) {
    // Pins the ALGORITHM, not just its properties. A save file stores a seed, so
    // changing the generator silently changes what every existing save replays.
    // If this test fails, that is a save-compatibility break and needs a
    // deliberate decision, not a quiet refactor.
    engine::Rng rng{ 42 };

    EXPECT_EQ(rng.nextUint32(), 0xa15c02b7u);
    EXPECT_EQ(rng.nextUint32(), 0x7b47f409u);
    EXPECT_EQ(rng.nextUint32(), 0xba1d3330u);
}

TEST(Rng, NextFloatIsInTheUnitIntervalExcludingOne) {
    // A value of exactly 1.0 breaks the common `table[int(r * size)]` idiom by
    // indexing one past the end.
    engine::Rng rng{ 7 };

    for (int i = 0; i < 10000; ++i) {
        const float value = rng.nextFloat();
        EXPECT_GE(value, 0.0f);
        EXPECT_LT(value, 1.0f);
    }
}

TEST(Rng, NextIntRespectsAnInclusiveRange) {
    engine::Rng rng{ 3 };

    for (int i = 0; i < 10000; ++i) {
        const int value = rng.nextInt(5, 10);
        EXPECT_GE(value, 5);
        EXPECT_LE(value, 10);
    }
}

TEST(Rng, NextIntCanReturnBothEndsOfTheRange) {
    // An off-by-one in the bounds maths usually shows up as one end never
    // appearing, which a range check alone would not catch.
    engine::Rng rng{ 11 };
    std::set<int> seen;

    for (int i = 0; i < 5000; ++i) {
        seen.insert(rng.nextInt(0, 3));
    }

    EXPECT_EQ(seen.size(), 4u);
    EXPECT_TRUE(seen.contains(0));
    EXPECT_TRUE(seen.contains(3));
}

TEST(Rng, NextIntWithASingleValueRangeAlwaysReturnsIt) {
    engine::Rng rng{ 13 };
    for (int i = 0; i < 100; ++i) {
        EXPECT_EQ(rng.nextInt(4, 4), 4);
    }
}

TEST(Rng, IsRoughlyUniform) {
    // The spread IS the behaviour for drop tables, so assert the distribution
    // rather than only individual draws (game_test_protocol.md).
    engine::Rng rng{ 2024 };
    constexpr int kBuckets = 10;
    constexpr int kSamples = 100000;
    int counts[kBuckets] = {};

    for (int i = 0; i < kSamples; ++i) {
        counts[rng.nextInt(0, kBuckets - 1)] += 1;
    }

    constexpr int kExpected = kSamples / kBuckets;
    for (int bucket = 0; bucket < kBuckets; ++bucket) {
        // Generous band: this catches a broken generator, not a slightly
        // unlucky one. A tolerance tight enough to flake would be worse than
        // no assertion at all.
        EXPECT_GT(counts[bucket], kExpected * 0.9);
        EXPECT_LT(counts[bucket], kExpected * 1.1);
    }
}

TEST(Rng, TwoInstancesDoNotShareState) {
    // Seam 4: no hidden global state. Two generators in one process must be
    // completely independent, or two tests could contaminate each other.
    engine::Rng first{ 100 };
    engine::Rng second{ 100 };

    (void)firstN(first, 10); // advance only the first

    EXPECT_NE(first.nextUint32(), second.nextUint32());
}

} // namespace
