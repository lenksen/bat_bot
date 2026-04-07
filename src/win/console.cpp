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

} // namespace

void InitializeConsole() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    const std::wstring title(constants::kAppTitle.begin(), constants::kAppTitle.end());
    SetConsoleTitleW(title.c_str());
}

void Write(const std::wstring& text) {
    DWORD written = 0;
    WriteConsoleW(GetOutHandle(), text.c_str(), static_cast<DWORD>(text.size()), &written, nullptr);
}

void WriteLine(const std::wstring& text) {
    Write(text);
    Write(L"\r\n");
}

std::wstring ReadLine() {
    wchar_t buffer[2048] = {};
    DWORD read = 0;
    if (!ReadConsoleW(GetInHandle(), buffer, static_cast<DWORD>(std::size(buffer) - 1), &read, nullptr)) {
        return {};
    }
    std::wstring line(buffer, buffer + read);
    while (!line.empty() && (line.back() == L'\n' || line.back() == L'\r')) {
        line.pop_back();
    }
    return line;
}

} // namespace wg::win
