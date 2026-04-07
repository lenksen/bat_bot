#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace wg {

// 描述目标文件已存在时的处理方式。
enum class OverwriteMode {
    Ask,
    Overwrite,
    Skip,
};

// 描述 ASK 模式是否已经被用户升级成“后续全部覆盖”或“后续全部跳过”。
enum class AskEscalation {
    None,
    OverwriteAll,
    SkipAll,
};

// 一次解压写入过程中共享的覆盖状态。
// mode 表示初始覆盖策略，escalation 表示在交互过程中是否升级成批量策略。
struct OverwriteState {
    OverwriteMode mode = OverwriteMode::Ask;
    AskEscalation escalation = AskEscalation::None;
};

// 描述一块本地固定磁盘，以及它是否为系统盘。
// 自动搜索时会利用这个信息调整搜索顺序。
struct DriveObject {
    std::filesystem::path root;
    std::filesystem::path rootPath;
    bool isSystem = false;
};

// 表示一个候选目标路径以及该候选项来自哪一类搜索来源。
struct TargetCandidate {
    std::filesystem::path path;
    std::wstring source;
};

// 记录归档写入结果统计。
// 这里只统计文件条目，不统计目录条目。
struct ExtractStats {
    int total = 0;
    int added = 0;
    int overwritten = 0;
    int skipped = 0;
};

// 记录归档提取阶段的完整结果。
// 其中 manifest 是后处理阶段的关键边界：
// 只有本次实际成功写入的文件才会出现在里面。
struct ExtractResult {
    std::vector<std::filesystem::path> manifest;
    ExtractStats stats;
    std::filesystem::path gameDir;
};

// 记录后处理阶段执行结果的摘要。
struct PostProcessResult {
    int botWritten = 0;
    int renamed = 0;
};

// 一次应用运行期间共享的文件系统上下文。
// 这些路径在运行开始时统一计算，后续模块只读取，不再重复推导。
struct AppContext {
    std::filesystem::path exeDir;
    std::filesystem::path workDir;
    std::filesystem::path configFile;
    std::filesystem::path logFile;
};

// 从 config.ini 中读取出的配置值。
struct ConfigValues {
    std::optional<std::filesystem::path> lastPath;
};

// 从进程入口解析出的启动参数。
struct RunOptions {
    std::optional<std::filesystem::path> initialArchivePath;
};

// 为未来可能的归档格式分支保留的类型。
enum class ArchiveFormat {
    Zip,
    Rar,
    Unknown,
};

// 提取层真正需要的最小归档条目元数据。
// 当前只关心条目路径和是否为目录。
struct ArchiveEntry {
    std::string pathUtf8;
    bool isDirectory = false;
};

} // namespace wg
