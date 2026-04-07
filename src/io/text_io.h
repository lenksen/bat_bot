#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace wg::io {

// 文本读取辅助函数。
// 当前主要服务于 config.ini 这类体量很小、按行解析的文本文件。
// 读取 UTF-8 文本文件，并按逻辑行拆分。
// 如果行尾包含 Windows 风格的 \r\n，会移除残留的 \r。
std::vector<std::wstring> ReadLines(const std::filesystem::path& path);

} // namespace wg::io
