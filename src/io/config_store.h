#pragma once

#include <filesystem>

#include "core/types.h"

namespace wg {

// 负责读写程序目录下的简单 key=value 配置文件。
class ConfigStore {
public:
    // 读取配置文件；若文件不存在或内容缺失，则返回默认值结构。
    static ConfigValues Load(const std::filesystem::path& path);
    // 写入当前支持的配置项。
    static void Save(const std::filesystem::path& path, const ConfigValues& values);
};

} // namespace wg
