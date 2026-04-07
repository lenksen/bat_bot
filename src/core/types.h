#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace wg {

enum class OverwriteMode {
    Ask,
    Overwrite,
    Skip,
};

enum class AskEscalation {
    None,
    OverwriteAll,
    SkipAll,
};

struct OverwriteState {
    OverwriteMode mode = OverwriteMode::Ask;
    AskEscalation escalation = AskEscalation::None;
};

struct DriveObject {
    std::filesystem::path root;
    std::filesystem::path rootPath;
    bool isSystem = false;
};

struct TargetCandidate {
    std::filesystem::path path;
    std::wstring source;
};

struct ExtractStats {
    int total = 0;
    int added = 0;
    int overwritten = 0;
    int skipped = 0;
};

struct ExtractResult {
    std::vector<std::filesystem::path> manifest;
    ExtractStats stats;
    std::filesystem::path gameDir;
};

struct PostProcessResult {
    int botWritten = 0;
    int renamed = 0;
};

struct AppContext {
    std::filesystem::path exeDir;
    std::filesystem::path workDir;
    std::filesystem::path configFile;
    std::filesystem::path logFile;
};

struct ConfigValues {
    std::optional<std::filesystem::path> lastPath;
};

struct RunOptions {
    std::optional<std::filesystem::path> initialArchivePath;
};

enum class ArchiveFormat {
    Zip,
    Rar,
    Unknown,
};

struct ArchiveEntry {
    std::string pathUtf8;
    bool isDirectory = false;
};

} // namespace wg
