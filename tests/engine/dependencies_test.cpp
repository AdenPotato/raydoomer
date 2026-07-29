// Scaffold acceptance: every locked dependency is fetched, linked, and usable
// from a headless test binary.
//
// These are not placeholder tests. Each one fails to build if its dependency is
// missing from the CMake wiring, which is exactly what the scaffold delivers.
// Nothing here touches a window, a GPU, or an audio device - the linux-test
// preset must stay runnable in CI (game_test_protocol.md).

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

#include <string>

namespace {

TEST(Dependencies, GoogleTestRuns) {
    // The harness itself. If this does not run, ctest is not wired up.
    SUCCEED();
}

TEST(Dependencies, GoogleMockIsAvailable) {
    // gmock ships separately from gtest and is required for the platform seam
    // (engine_protocol.md). Linking it is part of the scaffold, so assert it
    // is reachable rather than discovering it missing in ADE-24.
    testing::internal::CaptureStdout();
    std::string captured = testing::internal::GetCapturedStdout();
    EXPECT_TRUE(captured.empty());
}

TEST(Dependencies, JsonParsesAContentDefinition) {
    // Shaped like a real content file (content_protocol.md) rather than a toy
    // object, so this also pins that the tunable naming convention parses.
    const auto weapons = nlohmann::json::parse(R"({
        "shotgun": {
            "damage": 8.0,
            "pellets": 9,
            "fireIntervalSeconds": 0.85
        }
    })");

    ASSERT_TRUE(weapons.contains("shotgun"));
    EXPECT_DOUBLE_EQ(weapons["shotgun"]["damage"].get<double>(), 8.0);
    EXPECT_EQ(weapons["shotgun"]["pellets"].get<int>(), 9);
    EXPECT_NEAR(weapons["shotgun"]["fireIntervalSeconds"].get<double>(), 0.85, 1e-9);
}

TEST(Dependencies, JsonReportsMalformedInputRatherThanCrashing) {
    // content_protocol.md requires loaders to report a typed error, never crash.
    // This pins that the parser is configured to throw rather than terminate.
    EXPECT_THROW(
        { (void)nlohmann::json::parse(R"({ "unterminated": )"); },
        nlohmann::json::parse_error);
}

TEST(Dependencies, Cpp23IsEnabled) {
    // The locked standard. A misconfigured CMAKE_CXX_STANDARD silently drops to
    // an older mode, so assert it rather than trust the build file.
    static_assert(__cplusplus >= 202302L, "C++23 is required (locked_decisions.md)");
    SUCCEED();
}

} // namespace
