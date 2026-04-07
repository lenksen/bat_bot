#include "postprocess/post_processor.h"

#include <filesystem>

#include "core/constants.h"
#include "io/logger.h"
#include "ui/console_ui.h"
#include "win/encoding.h"

namespace wg::postprocess {

PostProcessResult Run(const Services& services, const std::vector<std::filesystem::path>& manifest, const std::wstring& botKey) {
    services.ui->Step(L"注入后处理");
    PostProcessResult result;
    services.logger->Debug(L"后处理 manifest 数量: " + std::to_wstring(manifest.size()));
    services.logger->Debug(L"BOT 卡密是否为空: " + std::to_wstring(botKey.empty() ? 1 : 0));

    // 后处理严格以 manifest 为边界，而不是重新扫描整个 Game 目录。
    // 这样可以避免误处理那些在本次运行中被跳过的旧文件。
    for (const auto& file : manifest) {
        if (!std::filesystem::is_regular_file(file)) {
            services.logger->Debug(L"跳过非普通文件: " + file.wstring());
            continue;
        }
        if (!botKey.empty() && _wcsicmp(file.filename().c_str(), std::wstring(constants::kUserIniName).c_str()) == 0) {
            win::WriteTextFileUtf8(file, win::WideToUtf8(botKey), false);
            result.botWritten++;
            services.logger->Debug(L"已写入 BOT 卡密: " + file.wstring());
        }
    }

    if (botKey.empty()) {
        services.ui->Info(L"已跳过 BOT 卡密写入");
    } else if (result.botWritten > 0) {
        services.ui->Success(L"已写入 BOT 卡密");
    } else {
        services.ui->Info(L"本次未找到可写入的 user.ini 文件");
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
            if (ec) {
                services.logger->Debug(L"移除旧 DLL 失败或不存在: " + dest.wstring() + L", ec=" + std::to_wstring(ec.value()));
            } else {
                services.logger->Debug(L"已移除旧 DLL: " + dest.wstring());
            }
            std::filesystem::rename(file, dest, ec);
            if (!ec) {
                result.renamed++;
                services.logger->Debug(L"已重命名 DLL: " + file.wstring() + L" -> " + dest.wstring());
            } else {
                services.logger->Debug(L"DLL 重命名失败: " + file.wstring() + L" -> " + dest.wstring() + L", ec=" + std::to_wstring(ec.value()));
            }
        }
    }

    if (result.renamed > 0) {
        services.ui->Success(L"已将 core_cn.dll 重命名为 hid.dll");
    } else {
        services.ui->Info(L"本次未找到需要重命名的 DLL 文件");
    }

    services.logger->Info(L"写入 BOT 卡密的 user.ini 数量: " + std::to_wstring(result.botWritten));
    services.logger->Info(L"重命名 core_cn.dll -> hid.dll 数量: " + std::to_wstring(result.renamed));
    return result;
}

} // namespace wg::postprocess
