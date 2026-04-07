#include "archive/archive_extractor.h"

#include <algorithm>

#include "archive/archive_reader.h"
#include "core/constants.h"
#include "core/errors.h"
#include "fs/path_utils.h"
#include "io/logger.h"
#include "ui/console_ui.h"
#include "ui/dialogs.h"
#include "win/encoding.h"

namespace wg::archive {

namespace {

// 归档前缀规则固定为英文路径，因此这里使用 ASCII 范围的小写转换即可。
// 这样能用最简单的方式实现大小写不敏感比较。
std::string ToLowerAscii(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool IsSupportedArchive(const std::filesystem::path& path) {
    const auto ext = ToLowerAscii(path.extension().string());
    return ext == ".zip" || ext == ".rar";
}

} // namespace

std::filesystem::path ResolveArchiveInput(const Services& services, const std::optional<std::filesystem::path>& initialArchive) {
    std::filesystem::path candidate;
    // 输入来源按优先级依次为：命令行启动参数、原生文件选择框、控制台手工输入。
    // 这样既兼容自动化调用，也兼容普通用户双击运行。
    if (initialArchive.has_value()) {
        services.logger->Debug(L"注入包来源: 命令行参数");
        candidate = *initialArchive;
    } else if (auto picked = ui::PickArchiveFile(services.app.exeDir); picked.has_value()) {
        services.logger->Debug(L"注入包来源: 原生文件选择框");
        candidate = *picked;
    } else {
        services.logger->Debug(L"注入包来源: 控制台手动输入");
        const std::wstring raw = services.ui->PromptLine(L"请输入注入包路径：");
        candidate = raw;
    }

    services.logger->Debug(L"注入包候选路径: " + candidate.wstring());

    if (candidate.empty() || !std::filesystem::exists(candidate)) {
        throw AppError("没有找到可用的注入包，请确认文件路径是否正确。");
    }
    if (!IsSupportedArchive(candidate)) {
        throw AppError("当前只支持 ZIP 或 RAR 格式的注入包。");
    }
    return std::filesystem::absolute(candidate);
}

ExtractResult ExtractGameArchiveSubset(const Services& services, const std::filesystem::path& archivePath, const std::filesystem::path& targetRoot, OverwriteMode overwriteMode, const std::filesystem::path& manifestFile) {
    services.ui->Step(L"压缩包检查");
    services.ui->Status(L"正在检查压缩包内容...");
    services.logger->Debug(L"开始检查压缩包: " + archivePath.wstring());
    services.logger->Debug(L"目标根目录: " + targetRoot.wstring());

    const auto gameDir = targetRoot / constants::kGameDirName;
    if (!std::filesystem::is_directory(gameDir)) {
        throw AppError(win::WideToUtf8(L"目标 Game 目录不存在: " + gameDir.wstring()));
    }
    services.logger->Debug(L"目标 Game 目录: " + gameDir.wstring());

    ArchiveReader reader(archivePath);
    const auto entries = reader.ListEntries();
    services.logger->Debug(L"归档条目数量: " + std::to_wstring(entries.size()));
    // 当前产品不是通用解压器。
    // 业务规则要求只把 league of legends/ 子树映射到目标 Game 目录。
    const std::string prefix = "league of legends/";

    OverwriteState overwriteState;
    overwriteState.mode = overwriteMode;

    ExtractResult result;
    result.gameDir = gameDir;

    bool foundSubset = false;
    bool writeStarted = false;
    for (const auto& entry : entries) {
        std::string normalized = entry.pathUtf8;
        std::replace(normalized.begin(), normalized.end(), '\\', '/');
        const std::string lowered = ToLowerAscii(normalized);
        if (lowered.rfind(prefix, 0) != 0) {
            services.logger->Debug(L"跳过非目标前缀条目: " + win::Utf8ToWide(normalized));
            continue;
        }

        foundSubset = true;
        services.logger->Debug(L"命中目标前缀条目: " + win::Utf8ToWide(normalized));
        const std::string relative = normalized.substr(prefix.size());
        if (relative.empty()) {
            services.logger->Debug(L"跳过根目录占位条目");
            continue;
        }

        const auto destination = fs::BuildSafeDestinationPath(gameDir, relative);
        services.logger->Debug(L"映射目标路径: " + destination.wstring());
        if (entry.isDirectory || (!normalized.empty() && normalized.back() == '/')) {
            // 目录条目只负责建立目录结构。
            // 后处理只关心文件，因此目录不会写入 manifest。
            std::filesystem::create_directories(destination);
            services.logger->Debug(L"创建目录: " + destination.wstring());
            continue;
        }

        if (!writeStarted) {
            services.ui->Success(L"已识别有效目录：league of legends/");
            services.ui->Step(L"文件写入");
            services.ui->Status(L"正在写入文件...");
            writeStarted = true;
        }

        result.stats.total++;
        const bool exists = std::filesystem::exists(destination);
        services.logger->Debug(L"文件写入前状态: exists=" + std::to_wstring(exists ? 1 : 0));
        if (!services.ui->ShouldWriteFile(overwriteState, destination)) {
            result.stats.skipped++;
            services.logger->Debug(L"用户选择跳过文件: " + destination.wstring());
            continue;
        }

        reader.ExtractFileTo(entry.pathUtf8, destination);
        if (exists) {
            result.stats.overwritten++;
            services.logger->Debug(L"已覆盖文件: " + destination.wstring());
        } else {
            result.stats.added++;
            services.logger->Debug(L"已新增文件: " + destination.wstring());
        }
        // manifest 是后续所有改动的唯一边界。
        // 只有真正写入成功的文件才允许进入后处理阶段。
        result.manifest.push_back(destination);
    }

    if (!foundSubset) {
        throw AppError("压缩包中未找到有效目录：league of legends/。");
    }

    if (!writeStarted) {
        services.ui->Success(L"已识别有效目录：league of legends/");
        services.ui->Step(L"文件写入");
        services.ui->Status(L"正在写入文件...");
    }

    result.manifest = fs::UniqueOrdered(result.manifest);
    std::vector<std::wstring> lines;
    for (const auto& item : result.manifest) {
        lines.push_back(item.wstring());
        services.logger->Debug(L"manifest 条目: " + item.wstring());
    }
    // 将本次实际写入的文件落盘保存，便于排障、人工核对和理解后处理边界。
    win::WriteLinesUtf8(manifestFile, lines, true);
    services.logger->Debug(L"manifest 文件已写入: " + manifestFile.wstring());

    services.ui->Success(L"文件写入完成");
    services.ui->Info(L"新增文件：" + std::to_wstring(result.stats.added));
    services.ui->Info(L"覆盖文件：" + std::to_wstring(result.stats.overwritten));
    services.ui->Info(L"跳过文件：" + std::to_wstring(result.stats.skipped));
    services.logger->Info(L"注入包写入完成: 新增=" + std::to_wstring(result.stats.added) + L"，覆盖=" + std::to_wstring(result.stats.overwritten) + L"，跳过=" + std::to_wstring(result.stats.skipped));
    return result;
}

} // namespace wg::archive
