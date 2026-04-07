#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace wg::fs {

// 路径辅助函数集合。
// 这里同时承担两类职责：
// 1. 目标目录合法性校验与规范化
// 2. 归档写入目标路径的安全构造
// 严格解析路径，用于错误提示和诊断输出。
// 仅当输入本身为空时视为错误；若规范化失败，则退回到绝对路径结果。
std::filesystem::path ResolveFullPathStrict(const std::filesystem::path& path, const std::wstring& name);
// 尽量把路径转换成绝对路径，但不把路径不存在当成致命错误。
std::optional<std::filesystem::path> NormalizePath(const std::filesystem::path& path);
// 校验体验服根目录是否合法。
// 合法定义为：目录本身存在，且其下存在 Game 子目录。
// 如果用户直接选中了 Game 目录，这里会自动回退到父目录作为标准根目录。
std::optional<std::filesystem::path> ValidateTargetRoot(const std::filesystem::path& path);
// 去重并保持原始发现顺序。
// 这样展示给用户的候选项顺序更稳定，也更贴近真实搜索过程。
std::vector<std::filesystem::path> UniqueOrdered(const std::vector<std::filesystem::path>& items);
// 归档写入的安全边界函数。
// 所有归档中的相对路径都必须经过这里，才能变成最终的磁盘写入目标。
// 任何未来新增的写入逻辑，也不应绕过这个函数直接构造目标路径。
std::filesystem::path BuildSafeDestinationPath(const std::filesystem::path& baseDir, const std::string& relativeUtf8);

} // namespace wg::fs
