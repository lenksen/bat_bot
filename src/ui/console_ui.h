#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "core/types.h"

namespace wg {

class Logger;

// 控制台交互层封装。
// 业务模块通过它输出提示和读取输入，避免把控制台格式细节散落在各个模块中。
class ConsoleUi {
public:
    // 绑定日志器，使关键阶段输出可以同时进入控制台和日志文件。
    explicit ConsoleUi(Logger& logger);

    // 用于展示流程状态、普通信息、警告和错误的基础输出接口。
    void ShowBanner() const;
    void Step(const std::wstring& message) const;
    void Info(const std::wstring& message) const;
    void Warn(const std::wstring& message) const;
    void Error(const std::wstring& message) const;

    // 读取用户输入，包括普通文本输入和覆盖策略输入。
    std::wstring PromptLine(const std::wstring& prompt) const;
    OverwriteMode PromptOverwriteMode() const;
    // 处理一次文件冲突的用户决策。
    // 若用户在 ASK 模式中选择“后续全部覆盖/跳过”，这里会同步更新状态。
    bool ShouldWriteFile(OverwriteState& state, const std::filesystem::path& destinationPath) const;
    // 让用户从候选目录列表中做选择。
    // 返回值语义：
    // 1. 普通路径：用户选择了某个候选项
    // 2. 空 path：用户要求切换到手动选择目录
    // 3. nullopt：当前没有候选项可展示
    std::optional<std::filesystem::path> PromptCandidateChoice(const std::vector<std::filesystem::path>& candidates, const std::wstring& title) const;

private:
    // 控制台 UI 依赖的日志器引用。
    Logger& logger_;
};

} // namespace wg
