// Filesystem access behind the platform seam.
//
// content_protocol.md: a loader validates at the boundary and reports a typed
// error naming what failed. It never crashes on bad input. This is the layer
// that makes that possible - the engine reports failure, the game decides the
// response (engine_protocol.md).

#include <gtest/gtest.h>

#include "platform/file_system.h"

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>

namespace {

// Writes a file into a unique temp directory and removes it afterwards, so the
// suite never depends on a committed fixture path or leaves anything behind.
class TempFile {
public:
    explicit TempFile(std::string contents) {
        dir_ = std::filesystem::temp_directory_path() /
               ("doomer_test_" + std::to_string(std::random_device{}()));
        std::filesystem::create_directories(dir_);
        path_ = dir_ / "content.json";
        std::ofstream out(path_, std::ios::binary);
        out << contents;
    }

    ~TempFile() {
        std::error_code ec;
        std::filesystem::remove_all(dir_, ec);
    }

    TempFile(const TempFile&) = delete;
    TempFile& operator=(const TempFile&) = delete;

    const std::filesystem::path& path() const { return path_; }

private:
    std::filesystem::path dir_;
    std::filesystem::path path_;
};

TEST(NativeFileSystem, ReadsAFileItCanFind) {
    const TempFile file(R"({"damage": 8.0})");
    platform::NativeFileSystem fs;

    const auto result = fs.read(file.path().string());

    ASSERT_TRUE(result.has_value()) << "expected a successful read";
    const std::string text(reinterpret_cast<const char*>(result->data()), result->size());
    EXPECT_EQ(text, R"({"damage": 8.0})");
}

TEST(NativeFileSystem, ReportsATypedErrorForAMissingFile) {
    platform::NativeFileSystem fs;

    const auto result = fs.read("/definitely/does/not/exist/weapons.json");

    ASSERT_FALSE(result.has_value()) << "a missing file must not read successfully";
    EXPECT_EQ(result.error().kind, platform::IoErrorKind::NotFound);
    // The message must name the path. An error that does not say which file
    // failed sends someone hunting through every content file.
    EXPECT_NE(result.error().message.find("weapons.json"), std::string::npos);
}

TEST(NativeFileSystem, DoesNotThrowOnAMissingFile) {
    // The engine reports failure; it does not decide the game's response, and
    // it certainly does not terminate. A missing asset is an operational error.
    platform::NativeFileSystem fs;
    EXPECT_NO_THROW({ (void)fs.read("/nope/missing.json"); });
}

TEST(NativeFileSystem, ReadsAnEmptyFileAsEmptyRatherThanAnError) {
    // An empty file is valid input, not a failure. Conflating the two makes
    // "the file is missing" and "the file is empty" indistinguishable.
    const TempFile file("");
    platform::NativeFileSystem fs;

    const auto result = fs.read(file.path().string());

    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->empty());
}

TEST(NativeFileSystem, IsUsableThroughTheInterface) {
    platform::NativeFileSystem concrete;
    platform::FileSystem& fs = concrete;
    EXPECT_FALSE(fs.read("/nope/missing.json").has_value());
}

} // namespace
