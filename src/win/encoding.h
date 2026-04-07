#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace wg::win {

std::string WideToUtf8(const std::wstring& value);
std::wstring Utf8ToWide(const std::string& value);
std::string ReadTextFileUtf8(const std::filesystem::path& path);
void WriteTextFileUtf8(const std::filesystem::path& path, const std::string& text, bool bom);
void WriteLinesUtf8(const std::filesystem::path& path, const std::vector<std::wstring>& lines, bool bom);

} // namespace wg::win
