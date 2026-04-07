#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace wg::win {

// 编码与 UTF-8 文本文件辅助函数。
// 统一封装宽字符与 UTF-8 之间的转换，以及常用文本文件的读写行为。
// 在 Windows 宽字符串与 UTF-8 字节串之间做转换。
// 路径和控制台交互主要使用宽字符串，日志和文本文件主要使用 UTF-8。
std::string WideToUtf8(const std::wstring& value);
std::wstring Utf8ToWide(const std::string& value);
// 读取 UTF-8 文本文件，并在存在 BOM 时自动剥离。
std::string ReadTextFileUtf8(const std::filesystem::path& path);
// 写入 UTF-8 文本，可按需写入 BOM。
void WriteTextFileUtf8(const std::filesystem::path& path, const std::string& text, bool bom);
// 面向按行写入场景的便捷封装。
void WriteLinesUtf8(const std::filesystem::path& path, const std::vector<std::wstring>& lines, bool bom);

} // namespace wg::win
