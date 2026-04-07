#include "io/logger.h"

#include <chrono>
#include <fstream>
#include <iomanip>
#include <sstream>

#include "win/encoding.h"

namespace wg {

namespace {

std::wstring CurrentTimestamp() {
    const auto now = std::chrono::system_clock::now();
    const auto tt = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
    localtime_s(&local, &tt);

    const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::wostringstream oss;
    oss << std::put_time(&local, L"%Y-%m-%d %H:%M:%S") << L'.' << std::setw(3) << std::setfill(L'0') << ms.count();
    return oss.str();
}

} // namespace

Logger::Logger(std::filesystem::path path) : path_(std::move(path)) {}

void Logger::Info(const std::wstring& message) const { Write(L"INFO", message); }
void Logger::Warn(const std::wstring& message) const { Write(L"WARN", message); }
void Logger::Error(const std::wstring& message) const { Write(L"ERROR", message); }

const std::filesystem::path& Logger::path() const noexcept { return path_; }

void Logger::Write(const std::wstring& level, const std::wstring& message) const {
    std::ofstream output(path_, std::ios::binary | std::ios::app);
    if (!output) {
        return;
    }
    const std::wstring line = L"[" + CurrentTimestamp() + L"] [" + level + L"] " + message + L"\r\n";
    const std::string utf8 = win::WideToUtf8(line);
    output.write(utf8.data(), static_cast<std::streamsize>(utf8.size()));
}

} // namespace wg
