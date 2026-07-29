#pragma once

#include <cstddef>
#include <expected>
#include <string>
#include <string_view>
#include <vector>

namespace platform {

/// Why a filesystem operation failed.
enum class IoErrorKind {
    NotFound,
    PermissionDenied,
    ReadFailed,
};

/// A typed failure from the filesystem.
///
/// @remarks
/// The engine reports failure; it does not decide the game's response. Whether
/// a missing asset is fatal is a game decision (engine_protocol.md). The message
/// always names the path, because an error that does not say which file failed
/// sends someone hunting through every content file.
struct IoError {
    IoErrorKind kind;
    std::string message;
};

/// The result of a read: bytes on success, a typed error on failure.
using ReadResult = std::expected<std::vector<std::byte>, IoError>;

/// Filesystem access, behind the platform seam.
///
/// @remarks
/// Gameplay never touches the filesystem directly ([gameplay_protocol.md]).
/// Everything goes through here so a test can substitute a fake and the suite
/// stays runnable with no disk layout to arrange.
class FileSystem {
public:
    virtual ~FileSystem() = default;

    /// Reads an entire file into memory.
    ///
    /// @param path Path to read. Interpreted by the implementation.
    /// @returns The file's bytes, or a typed error naming the path. An empty
    ///          file reads successfully as an empty vector - an empty file is
    ///          valid input, not a failure.
    /// @note Never throws for an absent or unreadable file. That is an expected
    ///       operational error, not a programmer error.
    virtual ReadResult read(std::string_view path) const = 0;
};

/// The real filesystem, backed by `std::filesystem` and `std::ifstream`.
///
/// @remarks
/// Deliberately free of any third-party dependency, so it compiles and is
/// tested in the headless `linux-test` configuration.
class NativeFileSystem final : public FileSystem {
public:
    ReadResult read(std::string_view path) const override;
};

} // namespace platform
