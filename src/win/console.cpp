#include "win/console.h"

#include <Windows.h>

#include <iterator>
#include <string>

#include "core/constants.h"

namespace wg::win {

namespace {

HANDLE GetOutHandle() {
    return GetStdHandle(STD_OUTPUT_HANDLE);
}

HANDLE GetInHandle() {
    return GetStdHandle(STD_INPUT_HANDLE);
}

WORD DefaultAttributes() {
    static WORD attrs = [] {
        CONSOLE_SCREEN_BUFFER_INFO info{};
        if (GetConsoleScreenBufferInfo(GetOutHandle(), &info)) {
            return info.wAttributes;
        }
        return static_cast<WORD>(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
    }();
    return attrs;
}

WORD ColorAttributes(Color color) {
    switch (color) {
    case Color::Dim:
        return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    case Color::Accent:
        return FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    case Color::Success:
        return FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    case Color::Warning:
        return FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    case Color::Error:
        return FOREGROUND_RED | FOREGROUND_INTENSITY;
    case Color::Default:
    default:
        return DefaultAttributes();
    }
}

void WriteWithColor(Color color, const std::wstring& text) {
    HANDLE out = GetOutHandle();
    CONSOLE_SCREEN_BUFFER_INFO info{};
    const bool hasInfo = GetConsoleScreenBufferInfo(out, &info) != 0;
    SetConsoleTextAttribute(out, ColorAttributes(color));
    DWORD written = 0;
    WriteConsoleW(out, text.c_str(), static_cast<DWORD>(text.size()), &written, nullptr);
    SetConsoleTextAttribute(out, hasInfo ? info.wAttributes : DefaultAttributes());
}

} // namespace

void InitializeConsole() {
    // 强制控制台输入输出使用 UTF-8，保证中文提示和粘贴路径时的行为一致。
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    const std::wstring title(constants::kAppTitle.begin(), constants::kAppTitle.end());
    SetConsoleTitleW(title.c_str());
}

void ClearScreen() {
    HANDLE out = GetOutHandle();
    CONSOLE_SCREEN_BUFFER_INFO info{};
    if (!GetConsoleScreenBufferInfo(out, &info)) {
        return;
    }

    const DWORD cellCount = static_cast<DWORD>(info.dwSize.X) * static_cast<DWORD>(info.dwSize.Y);
    COORD home{0, 0};
    DWORD written = 0;
    FillConsoleOutputCharacterW(out, L' ', cellCount, home, &written);
    FillConsoleOutputAttribute(out, info.wAttributes, cellCount, home, &written);
    SetConsoleCursorPosition(out, home);
}

void Write(const std::wstring& text) {
    DWORD written = 0;
    WriteConsoleW(GetOutHandle(), text.c_str(), static_cast<DWORD>(text.size()), &written, nullptr);
}

void WriteColored(Color color, const std::wstring& text) {
    WriteWithColor(color, text);
}

void WriteLine(const std::wstring& text) {
    Write(text);
    Write(L"\r\n");
}

void WriteLineColored(Color color, const std::wstring& text) {
    WriteWithColor(color, text);
    Write(L"\r\n");
}

std::wstring ReadLine() {
    wchar_t buffer[2048] = {};
    DWORD read = 0;
    if (!ReadConsoleW(GetInHandle(), buffer, static_cast<DWORD>(std::size(buffer) - 1), &read, nullptr)) {
        return {};
    }
    // ReadConsoleW 会把换行一起读回来，这里统一裁掉，
    // 让上层永远得到干净的一行输入文本。
    std::wstring line(buffer, buffer + read);
    while (!line.empty() && (line.back() == L'\n' || line.back() == L'\r')) {
        line.pop_back();
    }
    return line;
}

} // namespace wg::win
