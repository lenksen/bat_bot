#include "app/application.h"

#include <chrono>
#include <filesystem>
#include <iomanip>
#include <sstream>

#include <Windows.h>

#include "archive/archive_extractor.h"
#include "core/constants.h"
#include "core/context.h"
#include "core/errors.h"
#include "fs/target_resolver.h"
#include "io/config_store.h"
#include "io/logger.h"
#include "postprocess/post_processor.h"
#include "ui/console_ui.h"
#include "win/console.h"
#include "win/encoding.h"

namespace wg {

namespace {

std::wstring BuildTimestampForFileName() {
    const auto now = std::chrono::system_clock::now();
    const auto tt = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
    localtime_s(&local, &tt);
    std::wostringstream oss;
    oss << std::put_time(&local, L"%Y%m%d_%H%M%S");
    return oss.str();
}

std::filesystem::path GetExecutableDirectory() {
    wchar_t buffer[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return std::filesystem::current_path();
    }
    return std::filesystem::path(buffer).parent_path();
}

AppContext BuildContext() {
    const auto exeDir = GetExecutableDirectory();
    const auto stamp = BuildTimestampForFileName();
    const auto temp = std::filesystem::temp_directory_path() / constants::kTempRootName / stamp;
    std::filesystem::create_directories(temp);
    return {
        exeDir,
        temp,
        exeDir / constants::kConfigFileName,
        exeDir / (std::wstring(constants::kLogPrefix) + stamp + L".log"),
    };
}

} // namespace

int RunApplication(const RunOptions& options) {
    win::InitializeConsole();

    const AppContext app = BuildContext();
    Logger logger(app.logFile);
    ConsoleUi ui(logger);
    Services services{app, &logger, &ui};

    fs::CleanupOldWorkDirs(app.workDir.parent_path(), app.workDir);
    ui.ShowBanner();
    logger.Info(L"==== 运行开始 ====");
    logger.Info(L"程序目录: " + app.exeDir.wstring());
    logger.Info(L"工作目录: " + app.workDir.wstring());

    try {
        const auto archivePath = archive::ResolveArchiveInput(services, options.initialArchivePath);
        ui.Step(L"已选择BOT注入包");
        ui.Info(archivePath.wstring());
        logger.Info(L"压缩包文件: " + archivePath.wstring());

        const auto targetRoot = fs::ResolveTargetRoot(services);
        ui.Step(L"体验服目标目录已确定");
        ui.Info(targetRoot.wstring());

        const std::wstring botKey = ui.PromptLine(L"\n请输入 BOT 卡密（直接回车可跳过写入）: ");
        const OverwriteMode overwriteMode = ui.PromptOverwriteMode();

        const auto manifestFile = app.workDir / L"manifest.txt";
        const auto extractResult = archive::ExtractGameArchiveSubset(services, archivePath, targetRoot, overwriteMode, manifestFile);
        const auto postResult = postprocess::Run(services, extractResult.manifest, botKey);

        ConfigValues config;
        config.lastPath = targetRoot;
        ConfigStore::Save(app.configFile, config);
        logger.Info(L"已保存 last_path: " + targetRoot.wstring());

        ui.Step(L"注入完成");
        ui.Info(L"目标 Game 目录: " + extractResult.gameDir.wstring());
        ui.Info(L"新增文件: " + std::to_wstring(extractResult.stats.added));
        ui.Info(L"覆盖文件: " + std::to_wstring(extractResult.stats.overwritten));
        ui.Info(L"跳过文件: " + std::to_wstring(extractResult.stats.skipped));
        if (!botKey.empty()) {
            ui.Info(L"写入 BOT 卡密文件数: " + std::to_wstring(postResult.botWritten));
        }
        ui.Info(L"DLL 重命名数: " + std::to_wstring(postResult.renamed));
        ui.Info(L"日志文件: " + app.logFile.wstring());

        logger.Info(L"==== 运行成功结束 ====");
        std::error_code ec;
        std::filesystem::remove_all(app.workDir, ec);
        return 0;
    } catch (const std::exception& ex) {
        ui.Error(win::Utf8ToWide(ex.what()));
        logger.Error(win::Utf8ToWide(ex.what()));
        ui.Info(L"日志文件: " + app.logFile.wstring());
        std::error_code ec;
        std::filesystem::remove_all(app.workDir, ec);
        return 1;
    }
}

} // namespace wg
