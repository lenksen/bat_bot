#pragma once

#include <string>

namespace wg::win {

enum class Color {
    Default,
    Dim,
    Accent,
    Success,
    Warning,
    Error,
};

// Windows 控制台封装。
// 统一负责 UTF-8 编码设置、控制台标题以及宽字符输入输出。
// 初始化 Windows 控制台，设置 UTF-8 输入输出编码和控制台标题。
void InitializeConsole();
// 清空当前控制台内容并把光标移回左上角。
void ClearScreen();
// 宽字符控制台输出接口，供文本 UI 统一使用。
void WriteLine(const std::wstring& text);
void Write(const std::wstring& text);
void WriteLineColored(Color color, const std::wstring& text);
void WriteColored(Color color, const std::wstring& text);
// 从控制台读取一整行文本，并去掉 ReadConsoleW 带回的换行符。
std::wstring ReadLine();

} // namespace wg::win
