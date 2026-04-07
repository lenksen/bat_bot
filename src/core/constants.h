#pragma once

#include <string_view>

namespace wg::constants {

// 统一维护程序中的标题、关键目录名和固定文件名。
// 这些值既影响控制台展示，也影响路径拼接和业务规则判断，
// 放在一起定义可以避免不同模块各自硬编码造成不一致。
inline constexpr std::wstring_view kAppTitle = L"体验服BOT注入器";
inline constexpr std::wstring_view kBannerTitle = L" 体验服BOT注入器";
inline constexpr std::wstring_view kGameName = L"英雄联盟体验服";
inline constexpr std::wstring_view kGameDirName = L"Game";
inline constexpr std::wstring_view kConfigFileName = L"config.ini";
inline constexpr std::wstring_view kLogPrefix = L"tf_bot_injector_";
// 归档中只有此前缀下的内容允许写入目标 Game 目录。
// 这是当前 BOT 注入工具的核心业务规则，而不是通用解压规则。
inline constexpr std::wstring_view kArchivePrefix = L"league of legends/";
inline constexpr std::wstring_view kUserIniName = L"user.ini";
inline constexpr std::wstring_view kCoreDllName = L"core_cn.dll";
inline constexpr std::wstring_view kTargetDllName = L"hid.dll";
// 每次运行都会在系统临时目录下创建以此命名的工作根目录。
// 再在其下按时间戳创建本次运行专属目录，便于清理历史痕迹。
inline constexpr std::wstring_view kTempRootName = L"TfBotInjector";

} // namespace wg::constants
