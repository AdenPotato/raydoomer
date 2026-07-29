#pragma once

#include <cstdint>

namespace engine {

/// A seeded, reproducible random source.
///
/// @remarks
/// Seam 2 of the four (game_test_protocol.md). Nothing in the game reaches for
/// a global random source; an `Rng&` is passed in, so a test seeds it and gets
/// the same run every time.
///
/// **Deliberately not `<random>`.** `std::mt19937` is specified, but
/// `std::uniform_int_distribution` is not: the same seed produces different
/// values across standard library implementations. Since a save file stores an
/// RNG seed, that would make a save non-portable between a build made with
/// libstdc++ and one made with anything else. This is PCG32, implemented
/// explicitly, so the sequence is part of the project rather than the toolchain.
///
/// PCG32 by Melissa O'Neill (pcg-random.org). Small, fast, statistically sound,
/// and trivially reproducible.
class Rng {
public:
    /// @param seed Any value. The same seed always yields the same sequence.
    explicit Rng(uint64_t seed);

    /// Restarts the sequence from `seed`. Used when loading a save.
    void seed(uint64_t seed);

    /// @returns The next 32-bit value in the sequence.
    uint32_t nextUint32();

    /// @returns A value in [0, 1). Never reaches 1, which would index one past
    ///          the end of the common `table[int(r * size)]` idiom.
    float nextFloat();

    /// @returns A value in [min, max], inclusive at both ends.
    /// @note Uses rejection rather than a plain modulo, so low values are not
    ///       very slightly more likely than high ones. That bias is invisible in
    ///       a handful of draws and clearly wrong across a million loot rolls.
    int nextInt(int min, int max);

private:
    uint64_t state_ = 0;
    uint64_t increment_ = 0;
};

} // namespace engine
