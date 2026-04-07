#include "io/text_io.h"

#include <sstream>

#include "win/encoding.h"

namespace wg::io {

std::vector<std::wstring> ReadLines(const std::filesystem::path& path) {
    // 上层配置解析和路径处理主要使用宽字符串，
    // 因此这里在读入 UTF-8 后立即转换为宽字符，避免后续重复转换。
    const std::string utf8 = win::ReadTextFileUtf8(path);
    const std::wstring wide = win::Utf8ToWide(utf8);
    std::wstringstream ss(wide);
    std::vector<std::wstring> lines;
    std::wstring line;
    while (std::getline(ss, line)) {
        if (!line.empty() && line.back() == L'\r') {
            line.pop_back();
        }
        lines.push_back(line);
    }
    return lines;
}

} // namespace wg::io
