#include "io/text_io.h"

#include <sstream>

#include "win/encoding.h"

namespace wg::io {

std::vector<std::wstring> ReadLines(const std::filesystem::path& path) {
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
