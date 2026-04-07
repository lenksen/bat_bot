#pragma once

#include <filesystem>
#include <optional>

namespace wg::ui {

std::optional<std::filesystem::path> PickArchiveFile(const std::filesystem::path& initialDirectory);
std::optional<std::filesystem::path> PickFolder(const std::filesystem::path& initialDirectory);

} // namespace wg::ui
