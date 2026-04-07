#pragma once

#include <filesystem>
#include <string>

namespace wg {

// 轻量级追加式 UTF-8 日志器。
// 用于记录流程阶段、警告信息和错误信息，便于排障和用户反馈定位。
class Logger {
public:
    // 指定日志文件路径；文件在首次写入时创建。
    explicit Logger(std::filesystem::path path);

    // 记录不同级别的日志。
    void Info(const std::wstring& message) const;
    void Warn(const std::wstring& message) const;
    void Error(const std::wstring& message) const;

    // 返回日志文件路径，便于上层把日志位置展示给用户。
    const std::filesystem::path& path() const noexcept;

private:
    // 底层统一写入入口。
    void Write(const std::wstring& level, const std::wstring& message) const;

    // 目标日志文件路径。
    std::filesystem::path path_;
};

} // namespace wg
