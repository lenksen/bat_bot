#include "app/application.h"

#include <chrono>
#include <thread>
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

// 生成一个可排序的本地时间戳。
// 这个时间戳会同时用于日志文件名和临时工作目录名，
// 便于在排障时快速判断哪些文件属于同一次运行。
std::wstring BuildTimestampForFileName() {
    const auto now = std::chrono::system_clock::now();
    const auto tt = std::chrono::system_clock::to_time_t(now);
    std::tm local{};
    localtime_s(&local, &tt);
    std::wostringstream oss;
    oss << std::put_time(&local, L"%Y%m%d_%H%M%S");
    return oss.str();
}

// 获取当前可执行文件所在目录，而不是依赖当前工作目录。
// 配置文件、日志文件和文件选择对话框的默认位置都以程序目录为基准，
// 这样双击运行和命令行运行时行为更加一致。
std::filesystem::path GetExecutableDirectory() {
    wchar_t buffer[MAX_PATH] = {};
    const DWORD length = GetModuleFileNameW(nullptr, buffer, MAX_PATH);
    if (length == 0 || length >= MAX_PATH) {
        return std::filesystem::current_path();
    }
    return std::filesystem::path(buffer).parent_path();
}

// 构建一次运行期间共享的文件系统上下文。
// 包括程序目录、临时工作目录、配置文件路径和日志文件路径。
// 后续模块统一依赖这些路径，避免在不同地方重复推导同一批路径。
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
        // 主流程刻意保持线性展开，便于阅读和排障。
        // 其顺序直接对应业务步骤：选择注入包、定位目标目录、写入文件、执行后处理。
        ui.Step(L"注入包");
        const auto archivePath = archive::ResolveArchiveInput(services, options.initialArchivePath);
        ui.SetContextValue(L"注入包", archivePath.filename().wstring());
        ui.Info(L"已选择：" + archivePath.wstring());
        logger.Info(L"压缩包文件: " + archivePath.wstring());

        ui.Step(L"目标目录");
        const auto targetRoot = fs::ResolveTargetRoot(services);
        ui.SetContextValue(L"目标目录", targetRoot.wstring());
        ui.Info(L"当前使用：" + targetRoot.wstring());

        ui.Step(L"BOT 卡密");
        const std::wstring botKey = ui.PromptBotKey();
        if (botKey.empty()) {
            ui.SetContextValue(L"BOT 卡密", L"已跳过");
            ui.Info(L"已跳过");
        } else {
            ui.SetContextValue(L"BOT 卡密", L"已输入");
            ui.Info(L"已输入，将在注入后自动写入");
        }

        ui.Step(L"文件冲突处理");
        const OverwriteMode overwriteMode = ui.PromptOverwriteMode();
        if (overwriteMode == OverwriteMode::Ask) {
            ui.SetContextValue(L"冲突处理", L"逐个确认");
            ui.Info(L"冲突时逐个确认");
        } else if (overwriteMode == OverwriteMode::Overwrite) {
            ui.SetContextValue(L"冲突处理", L"全部覆盖");
            ui.Info(L"发现同名文件时全部覆盖");
        } else {
            ui.SetContextValue(L"冲突处理", L"全部跳过");
            ui.Info(L"发现同名文件时全部跳过");
        }

        const auto manifestFile = app.workDir / L"manifest.txt";
        const auto extractResult = archive::ExtractGameArchiveSubset(services, archivePath, targetRoot, overwriteMode, manifestFile);
        const auto postResult = postprocess::Run(services, extractResult.manifest, botKey);

        ConfigValues config;
        config.lastPath = targetRoot;
        ConfigStore::Save(app.configFile, config);
        logger.Info(L"已保存 last_path: " + targetRoot.wstring());

        ui.Step(L"结果摘要");
        ui.Success(L"注入完成");
        ui.Info(L"本次注入已成功结束，下面是结果摘要。");
        ui.Info(L"");
        ui.SummaryLine(L"状态", L"成功");
        ui.SummaryLine(L"目标目录", extractResult.gameDir.wstring());
        ui.SummaryLine(L"新增文件", std::to_wstring(extractResult.stats.added));
        ui.SummaryLine(L"覆盖文件", std::to_wstring(extractResult.stats.overwritten));
        ui.SummaryLine(L"跳过文件", std::to_wstring(extractResult.stats.skipped));
        ui.SummaryLine(L"卡密写入", std::to_wstring(postResult.botWritten));
        ui.SummaryLine(L"DLL 重命名", std::to_wstring(postResult.renamed));
        ui.SummaryLine(L"日志文件", app.logFile.wstring());
        std::this_thread::sleep_for(std::chrono::seconds(3));

        logger.Info(L"==== 运行成功结束 ====");
        // 临时工作目录只保存本次运行过程中的中间产物和辅助诊断信息。
        // 删除失败不应让原本成功的流程变成失败，因此这里采用 best-effort 清理。
        std::error_code ec;
        std::filesystem::remove_all(app.workDir, ec);
        return 0;
    } catch (const std::exception& ex) {
        ui.Step(L"结果摘要");
        ui.Error(L"注入失败");
        ui.Info(L"失败原因：");
        ui.Info(win::Utf8ToWide(ex.what()));
        ui.Info(L"日志文件：" + app.logFile.wstring());
        std::this_thread::sleep_for(std::chrono::seconds(3));
        logger.Error(win::Utf8ToWide(ex.what()));
        // 失败路径同样尝试清理临时工作目录，避免留下无意义的中间文件。
        // 这里也不能让清理失败覆盖原始业务错误。
        std::error_code ec;
        std::filesystem::remove_all(app.workDir, ec);
        return 1;
    }
}

} // namespace wg
