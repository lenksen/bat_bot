#include "io/config_store.h"

#include <vector>

#include "io/text_io.h"
#include "win/encoding.h"

namespace wg {

ConfigValues ConfigStore::Load(const std::filesystem::path& path) {
    ConfigValues values;
    if (!std::filesystem::exists(path)) {
        return values;
    }

    // 配置格式刻意保持简单：
    // 允许注释行，忽略未知键，缺失值则自然回退到默认行为。
    for (const auto& line : io::ReadLines(path)) {
        if (line.empty() || line[0] == L'#' || line[0] == L';') {
            continue;
        }
        const auto pos = line.find(L'=');
        if (pos == std::wstring::npos) {
            continue;
        }
        const std::wstring key = line.substr(0, pos);
        const std::wstring value = line.substr(pos + 1);
        if (key == L"last_path" && !value.empty()) {
            values.lastPath = std::filesystem::path(value);
        }
    }

    return values;
}

void ConfigStore::Save(const std::filesystem::path& path, const ConfigValues& values) {
    std::vector<std::wstring> lines;
    if (values.lastPath.has_value()) {
        lines.push_back(L"last_path=" + values.lastPath->wstring());
    }
    // 使用带 BOM 的 UTF-8 保存，保证 Windows 常见文本工具打开时中文路径不会乱码。
    win::WriteLinesUtf8(path, lines, true);
}

} // namespace wg
