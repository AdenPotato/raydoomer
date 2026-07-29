// The save model.
//
// runtime_architecture.md: saves are written at level boundaries and carry
// player stats, inventory, level progress, currency, and the RNG seed. Mid-level
// world state is never serialized.
//
// content_protocol.md requires a round-trip test and a migration test from each
// supported older version. There is currently exactly one version, so the set of
// older versions is empty - see the note on RejectsAVersionFromTheFuture below.
// The version machinery is built and its error paths are tested, so the first
// schema change adds a migration rather than inventing the concept.

#include <gtest/gtest.h>

#include "game/save.h"

#include <string>

namespace {

game::SaveData makeFullSave() {
    game::SaveData save;
    save.player.health = 73;
    save.player.maxHealth = 100;
    save.player.armor = 25;
    save.currency = 1234;
    save.rngSeed = 0xDEADBEEFCAFEULL;
    save.progress.currentLevel = 3;
    save.progress.completedLevels = { 1, 2 };
    save.inventory.stacks = {
        { game::ItemId{ 10 }, 2 },
        { game::ItemId{ 42 }, 1 },
    };
    return save;
}

TEST(Save, RoundTripsEveryPersistedField) {
    // The round-trip content_protocol.md requires. Every field asserted
    // individually: a memberwise comparison would silently pass if a field were
    // dropped from BOTH serialization and deserialization.
    const game::SaveData original = makeFullSave();

    const std::string json = game::toJson(original);
    const auto loaded = game::fromJson(json);

    ASSERT_TRUE(loaded.has_value()) << "round-trip failed to load";

    EXPECT_EQ(loaded->player.health, 73);
    EXPECT_EQ(loaded->player.maxHealth, 100);
    EXPECT_EQ(loaded->player.armor, 25);
    EXPECT_EQ(loaded->currency, 1234);
    EXPECT_EQ(loaded->rngSeed, 0xDEADBEEFCAFEULL);
    EXPECT_EQ(loaded->progress.currentLevel, 3);
    EXPECT_EQ(loaded->progress.completedLevels, (std::vector<int>{ 1, 2 }));
    ASSERT_EQ(loaded->inventory.stacks.size(), 2u);
    EXPECT_EQ(loaded->inventory.stacks[0].item.value, 10u);
    EXPECT_EQ(loaded->inventory.stacks[0].count, 2);
    EXPECT_EQ(loaded->inventory.stacks[1].item.value, 42u);
    EXPECT_EQ(loaded->inventory.stacks[1].count, 1);
}

TEST(Save, PreservesTheRngSeedExactly) {
    // The seed is what makes a loaded run replay identically. A float round-trip
    // or a narrowing conversion here would break reproducibility in a way that
    // only shows up as "loot feels different after loading".
    game::SaveData save;
    save.rngSeed = 0xFFFFFFFFFFFFFFFFULL;

    const auto loaded = game::fromJson(game::toJson(save));

    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->rngSeed, 0xFFFFFFFFFFFFFFFFULL);
}

TEST(Save, RoundTripsAnEmptyInventory) {
    game::SaveData save;
    save.inventory.stacks.clear();

    const auto loaded = game::fromJson(game::toJson(save));

    ASSERT_TRUE(loaded.has_value());
    EXPECT_TRUE(loaded->inventory.stacks.empty());
}

TEST(Save, WritesTheCurrentVersion) {
    const std::string json = game::toJson(game::SaveData{});
    EXPECT_NE(json.find("\"version\""), std::string::npos);

    const auto loaded = game::fromJson(json);
    ASSERT_TRUE(loaded.has_value());
    EXPECT_EQ(loaded->version, game::kCurrentSaveVersion);
}

TEST(Save, RejectsASaveWithNoVersion) {
    // A save without a version is unmigratable forever, so it must never be
    // accepted - not even by defaulting it to the current version, which would
    // silently misinterpret an older file.
    const auto loaded = game::fromJson(R"({"player":{"health":100}})");

    ASSERT_FALSE(loaded.has_value());
    EXPECT_EQ(loaded.error().kind, game::SaveErrorKind::MissingVersion);
}

TEST(Save, RejectsAVersionFromTheFuture) {
    // Loading a newer save on an older build must fail loudly rather than
    // silently dropping the fields it does not understand.
    //
    // This is also the test that will stop being vacuous: when kCurrentSaveVersion
    // becomes 2, a version-1 fixture gains a real migration test alongside it.
    const auto loaded = game::fromJson(R"({"version":9999})");

    ASSERT_FALSE(loaded.has_value());
    EXPECT_EQ(loaded.error().kind, game::SaveErrorKind::UnsupportedVersion);
}

TEST(Save, RejectsMalformedJsonWithATypedError) {
    // Never crash on bad input. A hand-edited save should fail validation
    // cleanly rather than corrupt the run (content_protocol.md).
    const auto loaded = game::fromJson(R"({"version": )");

    ASSERT_FALSE(loaded.has_value());
    EXPECT_EQ(loaded.error().kind, game::SaveErrorKind::Malformed);
}

TEST(Save, DoesNotThrowOnAnyMalformedInput) {
    for (const char* input : { "", "null", "[]", "{", R"({"version":"one"})", "12345" }) {
        EXPECT_NO_THROW({ (void)game::fromJson(input); }) << "input: " << input;
    }
}

TEST(Save, MissingOptionalFieldsFallBackToDefaultsRatherThanFailing) {
    // A save that predates a field is not corrupt. Defaulting is the migration
    // path for an additive change, and it must be deliberate rather than
    // incidental.
    const auto loaded = game::fromJson(R"({"version":1})");

    ASSERT_TRUE(loaded.has_value()) << "a minimal but valid save must load";
    EXPECT_EQ(loaded->player.health, game::PlayerState{}.health);
    EXPECT_TRUE(loaded->inventory.stacks.empty());
}

TEST(Save, ErrorMessagesAreUseful) {
    const auto loaded = game::fromJson(R"({"version":9999})");
    ASSERT_FALSE(loaded.has_value());
    EXPECT_FALSE(loaded.error().message.empty())
        << "an error with no message sends someone reading the parser";
}

} // namespace
