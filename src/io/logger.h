#pragma once

#include <filesystem>
#include <string>

namespace wg {

class Logger {
public:
    explicit Logger(std::filesystem::path path);

    void Info(const std::wstring& message) const;
    void Warn(const std::wstring& message) const;
    void Error(const std::wstring& message) const;

    const std::filesystem::path& path() const noexcept;

private:
    void Write(const std::wstring& level, const std::wstring& message) const;

    std::filesystem::path path_;
};

} // namespace wg
