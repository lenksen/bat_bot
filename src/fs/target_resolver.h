#pragma once

#include <filesystem>

#include "core/context.h"

namespace wg::fs {

// 目标目录解析层。
// 负责按预定搜索顺序定位体验服根目录，并对历史运行残留的临时工作目录做清理。
// 通过配置、自动搜索和人工选择等多阶段方式定位体验服根目录。
std::filesystem::path ResolveTargetRoot(const Services& services);
// 清理历史运行留下的旧临时工作目录。
// current 表示本次运行目录，不应被误删。
void CleanupOldWorkDirs(const std::filesystem::path& root, const std::filesystem::path& current);

} // namespace wg::fs
