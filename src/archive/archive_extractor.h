#pragma once

#include <filesystem>

#include "core/context.h"
#include "core/types.h"

namespace wg::archive {

// 归档处理层的业务入口。
// 这一层负责“解析输入来源”和“按业务规则提取指定子树”，
// 不负责底层归档格式解析细节，那部分由 ArchiveReader 处理。
// 按“命令行参数 -> 原生文件选择框 -> 控制台输入”的顺序解析 BOT 注入包路径。
std::filesystem::path ResolveArchiveInput(const Services& services, const std::optional<std::filesystem::path>& initialArchive);
// 在真正选择目标目录和写入文件前，先检查压缩包是否符合业务约束。
// 当前仅校验是否存在允许写入的 league of legends/ 子树。
std::vector<ArchiveEntry> ValidateArchive(const Services& services, const std::filesystem::path& archivePath);
// 将归档中允许写入的子树内容提取到目标 Game 目录，
// 同时记录本次实际成功写入的文件 manifest，供后处理阶段使用。
ExtractResult ExtractGameArchiveSubset(const Services& services, const std::filesystem::path& archivePath, const std::vector<ArchiveEntry>& entries, const std::filesystem::path& targetRoot, OverwriteMode overwriteMode, const std::filesystem::path& manifestFile);

} // namespace wg::archive
