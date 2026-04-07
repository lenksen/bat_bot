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
    if (initialArchive.has_value()) {
        candidate = *initialArchive;
    } else if (auto picked = ui::PickArchiveFile(services.app.exeDir); picked.has_value()) {
        candidate = *picked;
    } else {
        const std::wstring raw = services.ui->PromptLine(L"请输入BOT注入包路径: ");
        candidate = raw;
    }

    if (candidate.empty() || !std::filesystem::exists(candidate)) {
        throw AppError("BOT注入包不存在。");
    }
    if (!IsSupportedArchive(candidate)) {
        throw AppError("请选择 .zip 或 .rar 格式的BOT注入包。");
    }
    return std::filesystem::absolute(candidate);
}

ExtractResult ExtractGameArchiveSubset(const Services& services, const std::filesystem::path& archivePath, const std::filesystem::path& targetRoot, OverwriteMode overwriteMode, const std::filesystem::path& manifestFile) {
    services.ui->Step(L"开始写入BOT注入包内容");

    const auto gameDir = targetRoot / constants::kGameDirName;
    if (!std::filesystem::is_directory(gameDir)) {
        throw AppError(win::WideToUtf8(L"目标 Game 目录不存在: " + gameDir.wstring()));
    }

    ArchiveReader reader(archivePath);
    const auto entries = reader.ListEntries();
    const std::string prefix = "league of legends/";

    OverwriteState overwriteState;
    overwriteState.mode = overwriteMode;

    ExtractResult result;
    result.gameDir = gameDir;

    bool foundSubset = false;
    for (const auto& entry : entries) {
        std::string normalized = entry.pathUtf8;
        std::replace(normalized.begin(), normalized.end(), '\\', '/');
        const std::string lowered = ToLowerAscii(normalized);
        if (lowered.rfind(prefix, 0) != 0) {
            continue;
        }

        foundSubset = true;
        const std::string relative = normalized.substr(prefix.size());
        if (relative.empty()) {
            continue;
        }

        const auto destination = fs::BuildSafeDestinationPath(gameDir, relative);
        if (entry.isDirectory || (!normalized.empty() && normalized.back() == '/')) {
            std::filesystem::create_directories(destination);
            continue;
        }

        result.stats.total++;
        const bool exists = std::filesystem::exists(destination);
        if (!services.ui->ShouldWriteFile(overwriteState, destination)) {
            result.stats.skipped++;
            continue;
        }

        reader.ExtractFileTo(entry.pathUtf8, destination);
        if (exists) {
            result.stats.overwritten++;
        } else {
            result.stats.added++;
        }
        result.manifest.push_back(destination);
    }

    if (!foundSubset) {
        throw AppError("BOT注入包中未找到 league of legends 根目录内容。");
    }

    result.manifest = fs::UniqueOrdered(result.manifest);
    std::vector<std::wstring> lines;
    for (const auto& item : result.manifest) {
        lines.push_back(item.wstring());
    }
    win::WriteLinesUtf8(manifestFile, lines, true);

    services.logger->Info(L"注入包写入完成: 新增=" + std::to_wstring(result.stats.added) + L"，覆盖=" + std::to_wstring(result.stats.overwritten) + L"，跳过=" + std::to_wstring(result.stats.skipped));
    return result;
}

} // namespace wg::archive
