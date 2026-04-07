#include "archive/archive_reader.h"

#include <archive.h>
#include <archive_entry.h>

#include <fstream>
#include <stdexcept>

#include "win/encoding.h"

namespace wg::archive {

ArchiveReader::ArchiveReader(const std::filesystem::path& archivePath) : archivePath_(archivePath) {}

std::vector<ArchiveEntry> ArchiveReader::ListEntries() const {
    std::vector<ArchiveEntry> out;
    // 每次操作都重新创建 reader，使这个封装保持无状态，
    // 避免跨函数共享 libarchive 状态导致行为不透明。
    struct archive* ar = archive_read_new();
    archive_read_support_filter_all(ar);
    archive_read_support_format_all(ar);

    if (archive_read_open_filename_w(ar, archivePath_.c_str(), 10240) != ARCHIVE_OK) {
        const std::string msg = archive_error_string(ar) ? archive_error_string(ar) : "failed to open archive";
        archive_read_free(ar);
        throw std::runtime_error(msg);
    }

    archive_entry* entry = nullptr;
    while (archive_read_next_header(ar, &entry) == ARCHIVE_OK) {
        const char* path = archive_entry_pathname_utf8(entry);
        if (!path) path = archive_entry_pathname(entry);
        out.push_back({path ? path : "", archive_entry_filetype(entry) == AE_IFDIR});
        archive_read_data_skip(ar);
    }

    archive_read_free(ar);
    return out;
}

void ArchiveReader::ExtractFileTo(const std::string& entryPathUtf8, const std::filesystem::path& destination) const {
    struct archive* ar = archive_read_new();
    archive_read_support_filter_all(ar);
    archive_read_support_format_all(ar);
    if (archive_read_open_filename_w(ar, archivePath_.c_str(), 10240) != ARCHIVE_OK) {
        const std::string msg = archive_error_string(ar) ? archive_error_string(ar) : "failed to open archive";
        archive_read_free(ar);
        throw std::runtime_error(msg);
    }

    archive_entry* entry = nullptr;
    bool matched = false;
    // 这里通过重新扫描归档定位目标条目。
    // 虽然不是随机访问实现，但逻辑简单清晰，且对当前一次性注入流程足够可控。
    while (archive_read_next_header(ar, &entry) == ARCHIVE_OK) {
        const char* path = archive_entry_pathname_utf8(entry);
        if (!path) path = archive_entry_pathname(entry);
        if (path && entryPathUtf8 == path) {
            std::filesystem::create_directories(destination.parent_path());
            std::ofstream output(destination, std::ios::binary | std::ios::trunc);
            if (!output) {
                archive_read_free(ar);
                throw std::runtime_error("failed to open destination file");
            }

            char buffer[16384];
            la_ssize_t size = 0;
            while ((size = archive_read_data(ar, buffer, sizeof(buffer))) > 0) {
                output.write(buffer, size);
            }
            matched = true;
            break;
        }
        archive_read_data_skip(ar);
    }

    archive_read_free(ar);
    if (!matched) {
        throw std::runtime_error("archive entry not found during extraction");
    }
}

} // namespace wg::archive
