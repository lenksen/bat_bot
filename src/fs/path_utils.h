#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace wg::fs {

std::filesystem::path ResolveFullPathStrict(const std::filesystem::path& path, const std::wstring& name);
std::optional<std::filesystem::path> NormalizePath(const std::filesystem::path& path);
std::optional<std::filesystem::path> ValidateTargetRoot(const std::filesystem::path& path);
std::vector<std::filesystem::path> UniqueOrdered(const std::vector<std::filesystem::path>& items);
std::filesystem::path BuildSafeDestinationPath(const std::filesystem::path& baseDir, const std::string& relativeUtf8);

} // namespace wg::fs
