#include "platform/file_system.h"

#include <filesystem>
#include <fstream>

namespace platform {
namespace {

IoError makeError(IoErrorKind kind, std::string_view path, std::string_view reason) {
    return IoError{ kind, std::string(reason) + ": " + std::string(path) };
}

} // namespace

ReadResult NativeFileSystem::read(std::string_view path) const {
    const std::filesystem::path fsPath{ path };

    // Validate once, at the boundary, and report rather than throw.
    std::error_code ec;
    if (!std::filesystem::exists(fsPath, ec) || ec) {
        return std::unexpected(makeError(IoErrorKind::NotFound, path, "file not found"));
    }
    if (std::filesystem::is_directory(fsPath, ec)) {
        return std::unexpected(makeError(IoErrorKind::ReadFailed, path, "path is a directory"));
    }

    std::ifstream in(fsPath, std::ios::binary);
    if (!in) {
        return std::unexpected(
            makeError(IoErrorKind::PermissionDenied, path, "could not open file"));
    }

    const auto size = std::filesystem::file_size(fsPath, ec);
    if (ec) {
        return std::unexpected(makeError(IoErrorKind::ReadFailed, path, "could not size file"));
    }

    std::vector<std::byte> bytes(static_cast<size_t>(size));
    if (size > 0) {
        in.read(reinterpret_cast<char*>(bytes.data()), static_cast<std::streamsize>(size));
        if (!in) {
            return std::unexpected(makeError(IoErrorKind::ReadFailed, path, "read failed"));
        }
    }

    return bytes;
}

} // namespace platform
