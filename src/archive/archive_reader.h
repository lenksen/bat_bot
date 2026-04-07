#pragma once

#include <filesystem>
#include <vector>

#include "core/types.h"

namespace wg::archive {

// 对 libarchive 的轻量封装。
// 业务层只关心“列出归档条目”和“提取单个条目”，不直接感知底层库细节。
class ArchiveReader {
public:
    // 绑定一个待处理的归档文件路径。
    explicit ArchiveReader(const std::filesystem::path& archivePath);
    // 枚举归档中的条目信息，但不真正解压文件内容。
    std::vector<ArchiveEntry> ListEntries() const;
    // 按归档内的精确路径提取单个条目到指定目标路径。
    void ExtractFileTo(const std::string& entryPathUtf8, const std::filesystem::path& destination) const;

private:
    // 当前 reader 绑定的归档文件路径。
    std::filesystem::path archivePath_;
};

} // namespace wg::archive
