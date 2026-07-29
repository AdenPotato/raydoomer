#include "engine/rng.h"

namespace engine {
namespace {

// PCG32 constants, from the reference implementation at pcg-random.org.
constexpr uint64_t kMultiplier = 6364136223846793005ULL;

// The stream selector. 54 is the value used by the reference demo, so the output
// for a given seed can be checked against upstream rather than only against
// ourselves.
constexpr uint64_t kStreamSelector = 54ULL;

} // namespace

Rng::Rng(uint64_t initialSeed) {
    seed(initialSeed);
}

void Rng::seed(uint64_t initialSeed) {
    state_ = 0U;
    increment_ = (kStreamSelector << 1u) | 1u;
    (void)nextUint32();
    state_ += initialSeed;
    (void)nextUint32();
}

uint32_t Rng::nextUint32() {
    const uint64_t previous = state_;
    state_ = previous * kMultiplier + increment_;

    const auto xorshifted = static_cast<uint32_t>(((previous >> 18u) ^ previous) >> 27u);
    const auto rotation = static_cast<uint32_t>(previous >> 59u);
    return (xorshifted >> rotation) | (xorshifted << ((~rotation + 1u) & 31u));
}

float Rng::nextFloat() {
    // 24 bits of mantissa, scaled into [0, 1). Dividing by 2^32 instead can
    // round up to exactly 1.0 in float.
    return static_cast<float>(nextUint32() >> 8) * 0x1.0p-24f;
}

int Rng::nextInt(int min, int max) {
    if (min >= max) {
        return min;
    }

    const auto range = static_cast<uint32_t>(max - min) + 1u;

    // Rejection sampling. Values below the threshold would map unevenly onto the
    // range, making low results marginally more likely - invisible in a few
    // draws, clearly wrong across a million loot rolls.
    const uint32_t threshold = (~range + 1u) % range;
    for (;;) {
        const uint32_t value = nextUint32();
        if (value >= threshold) {
            return min + static_cast<int>(value % range);
        }
    }
}

} // namespace engine
