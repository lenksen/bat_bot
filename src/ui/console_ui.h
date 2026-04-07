#pragma once

#include <filesystem>
#include <map>
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

    // 用于展示流程标题、说明、状态和结果摘要的统一输出接口。
    void ShowBanner() const;
    // 输出一个新的流程阶段标题，例如“注入包”“目标目录”。
    void Step(const std::wstring& title) const;
    // 输出普通说明文字，通常作为某个阶段下的补充信息。
    void Info(const std::wstring& message) const;
    // 输出进行中的状态提示，例如“正在检查...”或“正在写入...”。
    void Status(const std::wstring& message) const;
    // 输出成功结果，统一使用更醒目的成功前缀。
    void Success(const std::wstring& message) const;
    void Warn(const std::wstring& message) const;
    void Error(const std::wstring& message) const;
    // 结果摘要中的单行键值输出，保持最终统计展示风格一致。
    void SummaryLine(const std::wstring& key, const std::wstring& value) const;
    // 输出一个独立的交互面板标题，用于目录选择、冲突处理等输入界面。
    void PromptTitle(const std::wstring& title) const;
    // 更新顶部上下文区中的关键信息，例如已选注入包、目标目录、覆盖策略。
    void SetContextValue(const std::wstring& key, const std::wstring& value);

    // 读取用户输入，包括普通文本输入和覆盖策略输入。
    std::wstring PromptLine(const std::wstring& prompt) const;
    // 以统一的中文引导界面读取 BOT 卡密，支持直接回车跳过。
    std::wstring PromptBotKey() const;
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
    std::wstring NormalizeInlineValue(const std::wstring& value) const;
    std::wstring BuildLabelValueLine(const std::wstring& key, const std::wstring& value, size_t labelWidth) const;
    // 进入新阶段时重绘整屏，上方保留已确认的关键上下文信息。
    void RenderStageFrame(const std::wstring& title) const;

    // 控制台 UI 依赖的日志器引用。
    Logger& logger_;
    std::map<std::wstring, std::wstring> contextValues_;
};

} // namespace wg
