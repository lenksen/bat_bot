#pragma once

#include <filesystem>

#include "core/context.h"

namespace wg::fs {

std::filesystem::path ResolveTargetRoot(const Services& services);
void CleanupOldWorkDirs(const std::filesystem::path& root, const std::filesystem::path& current);

} // namespace wg::fs
