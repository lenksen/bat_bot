#pragma once

#include <filesystem>

#include "core/types.h"

namespace wg {

class ConfigStore {
public:
    static ConfigValues Load(const std::filesystem::path& path);
    static void Save(const std::filesystem::path& path, const ConfigValues& values);
};

} // namespace wg
