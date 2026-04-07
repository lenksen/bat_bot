#pragma once

#include <string>

namespace wg::win {

void InitializeConsole();
void WriteLine(const std::wstring& text);
void Write(const std::wstring& text);
std::wstring ReadLine();

} // namespace wg::win
