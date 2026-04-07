#pragma once

#include <filesystem>
#include <vector>

#include "core/types.h"

namespace wg::archive {

class ArchiveReader {
public:
    explicit ArchiveReader(const std::filesystem::path& archivePath);
    std::vector<ArchiveEntry> ListEntries() const;
    void ExtractFileTo(const std::string& entryPathUtf8, const std::filesystem::path& destination) const;

private:
    std::filesystem::path archivePath_;
};

} // namespace wg::archive
