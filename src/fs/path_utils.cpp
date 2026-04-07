#include "fs/path_utils.h"

#include <unordered_set>

#include "core/constants.h"
#include "core/errors.h"
#include "win/encoding.h"

namespace wg::fs {

std::filesystem::path ResolveFullPathStrict(const std::filesystem::path& path, const std::wstring& name) {
    if (path.empty()) {
        throw AppError(win::WideToUtf8(name + L" 为空。"));
    }
    std::error_code ec;
    const auto full = std::filesystem::weakly_canonical(path, ec);
    if (ec) {
        return std::filesystem::absolute(path);
    }
    return full;
}

std::optional<std::filesystem::path> NormalizePath(const std::filesystem::path& path) {
    if (path.empty()) {
        return std::nullopt;
    }
    std::error_code ec;
    const auto full = std::filesystem::absolute(path, ec);
    if (ec) {
        return std::nullopt;
    }
    return full;
}

std::optional<std::filesystem::path> ValidateTargetRoot(const std::filesystem::path& path) {
    auto full = NormalizePath(path);
    if (!full.has_value()) {
        return std::nullopt;
    }

    auto normalized = *full;
    if (normalized.filename() == constants::kGameDirName) {
        normalized = normalized.parent_path();
    }

    const auto gameDir = normalized / constants::kGameDirName;
    if (std::filesystem::is_directory(normalized) && std::filesystem::is_directory(gameDir)) {
        return normalized;
    }
    return std::nullopt;
}

std::vector<std::filesystem::path> UniqueOrdered(const std::vector<std::filesystem::path>& items) {
    std::unordered_set<std::wstring> seen;
    std::vector<std::filesystem::path> out;
    for (const auto& item : items) {
        const std::wstring key = item.wstring();
        if (seen.insert(key).second) {
            out.push_back(item);
        }
    }
    return out;
}

std::filesystem::path BuildSafeDestinationPath(const std::filesystem::path& baseDir, const std::string& relativeUtf8) {
    std::wstring relative = win::Utf8ToWide(relativeUtf8);
    for (auto& ch : relative) {
        if (ch == L'/') {
            ch = L'\\';
        }
    }
    while (!relative.empty() && (relative.front() == L'\\' || relative.front() == L'/')) {
        relative.erase(relative.begin());
    }
    if (relative.empty()) {
        throw AppError("archive entry path is empty");
    }
    if (relative.find(L"..\\") != std::wstring::npos || relative == L"..") {
        throw AppError(win::WideToUtf8(L"压缩包条目包含非法相对路径: " + relative));
    }

    const auto baseFull = std::filesystem::absolute(baseDir).lexically_normal();
    const auto destFull = std::filesystem::absolute(baseDir / relative).lexically_normal();
    std::wstring base = baseFull.wstring();
    std::wstring dest = destFull.wstring();
    if (!base.empty() && base.back() != L'\\') {
        base.push_back(L'\\');
    }
    if (!(dest == baseFull.wstring() || dest.rfind(base, 0) == 0)) {
        throw AppError(win::WideToUtf8(L"压缩包条目越界: " + relative));
    }
    return destFull;
}

} // namespace wg::fs
