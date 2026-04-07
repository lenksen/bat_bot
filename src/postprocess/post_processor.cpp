#include "postprocess/post_processor.h"

#include <filesystem>

#include "core/constants.h"
#include "io/logger.h"
#include "ui/console_ui.h"
#include "win/encoding.h"

namespace wg::postprocess {

PostProcessResult Run(const Services& services, const std::vector<std::filesystem::path>& manifest, const std::wstring& botKey) {
    services.ui->Step(L"执行注入后处理");
    PostProcessResult result;

    // 后处理严格以 manifest 为边界，而不是重新扫描整个 Game 目录。
    // 这样可以避免误处理那些在本次运行中被跳过的旧文件。
    for (const auto& file : manifest) {
        if (!std::filesystem::is_regular_file(file)) {
            continue;
        }
        if (!botKey.empty() && _wcsicmp(file.filename().c_str(), std::wstring(constants::kUserIniName).c_str()) == 0) {
            win::WriteTextFileUtf8(file, win::WideToUtf8(botKey), false);
            result.botWritten++;
        }
    }

    // 重命名单独放在第二轮执行。
    // 这样前一轮处理 user.ini 时仍然面对原始文件名，最终再统一调整 DLL 命名。
    for (const auto& file : manifest) {
        if (!std::filesystem::is_regular_file(file)) {
            continue;
        }
        if (_wcsicmp(file.filename().c_str(), std::wstring(constants::kCoreDllName).c_str()) == 0) {
            const auto dest = file.parent_path() / constants::kTargetDllName;
            std::error_code ec;
            std::filesystem::remove(dest, ec);
            std::filesystem::rename(file, dest, ec);
            if (!ec) {
                result.renamed++;
            }
        }
    }

    services.logger->Info(L"写入 BOT 卡密的 user.ini 数量: " + std::to_wstring(result.botWritten));
    services.logger->Info(L"重命名 core_cn.dll -> hid.dll 数量: " + std::to_wstring(result.renamed));
    return result;
}

} // namespace wg::postprocess
