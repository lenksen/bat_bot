#include "win/encoding.h"

#include <Windows.h>

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace wg::win {

namespace {

std::string ReadBinary(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("failed to open file for reading");
    }
    std::ostringstream oss;
    oss << input.rdbuf();
    return oss.str();
}

} // namespace

std::string WideToUtf8(const std::wstring& value) {
    if (value.empty()) {
        return {};
    }
    const int size = WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    std::string out(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), out.data(), size, nullptr, nullptr);
    return out;
}

std::wstring Utf8ToWide(const std::string& value) {
    if (value.empty()) {
        return {};
    }
    const int size = MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), nullptr, 0);
    std::wstring out(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, value.c_str(), static_cast<int>(value.size()), out.data(), size);
    return out;
}

std::string ReadTextFileUtf8(const std::filesystem::path& path) {
    std::string bytes = ReadBinary(path);
    if (bytes.size() >= 3 && static_cast<unsigned char>(bytes[0]) == 0xEF && static_cast<unsigned char>(bytes[1]) == 0xBB && static_cast<unsigned char>(bytes[2]) == 0xBF) {
        return bytes.substr(3);
    }
    return bytes;
}

void WriteTextFileUtf8(const std::filesystem::path& path, const std::string& text, bool bom) {
    std::ofstream output(path, std::ios::binary | std::ios::trunc);
    if (!output) {
        throw std::runtime_error("failed to open file for writing");
    }
    if (bom) {
        static constexpr unsigned char utf8Bom[] = {0xEF, 0xBB, 0xBF};
        output.write(reinterpret_cast<const char*>(utf8Bom), sizeof(utf8Bom));
    }
    output.write(text.data(), static_cast<std::streamsize>(text.size()));
}

void WriteLinesUtf8(const std::filesystem::path& path, const std::vector<std::wstring>& lines, bool bom) {
    std::string text;
    for (size_t i = 0; i < lines.size(); ++i) {
        text += WideToUtf8(lines[i]);
        text += "\r\n";
    }
    WriteTextFileUtf8(path, text, bom);
}

} // namespace wg::win
