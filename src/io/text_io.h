#pragma once

#include <filesystem>
#include <string>
#include <vector>

namespace wg::io {

std::vector<std::wstring> ReadLines(const std::filesystem::path& path);

} // namespace wg::io
