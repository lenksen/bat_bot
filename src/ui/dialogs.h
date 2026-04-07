#pragma once

#include <filesystem>
#include <optional>

namespace wg::ui {

// 原生文件/目录选择对话框封装。
// 这一层只负责与 Windows IFileDialog 交互，不负责业务校验和后续处理。
// 打开原生文件选择框，并限制为支持的 BOT 注入包格式。
std::optional<std::filesystem::path> PickArchiveFile(const std::filesystem::path& initialDirectory);
// 打开原生目录选择框，用于选择体验服根目录或其 Game 目录。
std::optional<std::filesystem::path> PickFolder(const std::filesystem::path& initialDirectory);

} // namespace wg::ui
