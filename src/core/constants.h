#pragma once

#include <string_view>

namespace wg::constants {

inline constexpr std::wstring_view kAppTitle = L"体验服BOT注入器";
inline constexpr std::wstring_view kBannerTitle = L" 体验服BOT注入器";
inline constexpr std::wstring_view kGameName = L"英雄联盟体验服";
inline constexpr std::wstring_view kGameDirName = L"Game";
inline constexpr std::wstring_view kConfigFileName = L"config.ini";
inline constexpr std::wstring_view kLogPrefix = L"tf_bot_injector_";
inline constexpr std::wstring_view kArchivePrefix = L"league of legends/";
inline constexpr std::wstring_view kUserIniName = L"user.ini";
inline constexpr std::wstring_view kCoreDllName = L"core_cn.dll";
inline constexpr std::wstring_view kTargetDllName = L"hid.dll";
inline constexpr std::wstring_view kTempRootName = L"TfBotInjector";

} // namespace wg::constants
