#include "ui/console_ui.h"

#include <cwchar>
#include <string>

#include "core/constants.h"
#include "io/logger.h"
#include "win/console.h"

namespace wg {

ConsoleUi::ConsoleUi(Logger& logger) : logger_(logger) {}

namespace {

constexpr const wchar_t* kContextOrder[] = {
    L"注入包",
    L"目标目录",
    L"BOT 卡密",
    L"冲突处理",
};

size_t DisplayWidth(const std::wstring& text) {
    size_t width = 0;
    for (wchar_t ch : text) {
        width += (ch >= 0x2E80) ? 2u : 1u;
    }
    return width;
}

std::wstring PadRightDisplay(const std::wstring& text, size_t targetWidth) {
    std::wstring result = text;
    const size_t width = DisplayWidth(text);
    if (width < targetWidth) {
        result.append(targetWidth - width, L' ');
    }
    return result;
}

} // namespace

std::wstring ConsoleUi::NormalizeInlineValue(const std::wstring& value) const {
    std::wstring normalized = value;
    for (auto& ch : normalized) {
        if (ch == L'\r' || ch == L'\n' || ch == L'\t') {
            ch = L' ';
        }
    }
    return normalized;
}

std::wstring ConsoleUi::BuildLabelValueLine(const std::wstring& key, const std::wstring& value, size_t labelWidth) const {
    const std::wstring normalized = NormalizeInlineValue(value);
    return L"  " + PadRightDisplay(key, labelWidth) + L"  " + normalized;
}

// 启动标题保持两行极简结构，减少传统 CLI 粗分隔线带来的噪音。
// 第一行是工具名，第二行直接说明用途，方便普通用户快速确认程序作用。
void ConsoleUi::ShowBanner() const {
    win::ClearScreen();
    win::WriteLineColored(win::Color::Accent, std::wstring(constants::kAppTitle));
    win::WriteLineColored(win::Color::Dim, L"帮助你将注入包写入体验服 Game 目录");
    win::WriteLine(L"");
}

// Step 用来切换到新的流程阶段。
// 控制台中使用统一的“> 标题”样式，日志里则保留阶段文本便于回溯。
void ConsoleUi::Step(const std::wstring& title) const {
    RenderStageFrame(title);
    logger_.Info(L"阶段: " + title);
}

void ConsoleUi::Info(const std::wstring& message) const { win::WriteLineColored(win::Color::Default, L"  " + message); }

// Status 用于显示正在进行中的动作，语气保持中性，避免和最终结果混淆。
void ConsoleUi::Status(const std::wstring& message) const {
    win::WriteLineColored(win::Color::Accent, L"  " + message);
    logger_.Info(message);
}

// Success 只用于已经完成的正向结果，统一使用 OK 前缀增强阶段完成感。
void ConsoleUi::Success(const std::wstring& message) const {
    win::WriteLineColored(win::Color::Success, L"OK " + message);
    logger_.Info(message);
}

void ConsoleUi::Warn(const std::wstring& message) const {
    win::WriteLineColored(win::Color::Warning, L"! " + message);
    logger_.Warn(message);
}

void ConsoleUi::Error(const std::wstring& message) const {
    win::WriteLineColored(win::Color::Error, L"X " + message);
}

// SummaryLine 负责最终摘要的对齐输出，让成功结果区保持稳定的阅读节奏。
void ConsoleUi::SummaryLine(const std::wstring& key, const std::wstring& value) const {
    win::WriteLine(BuildLabelValueLine(key, value, 8));
}

void ConsoleUi::PromptTitle(const std::wstring& title) const {
    win::WriteLine(L"");
    win::WriteLineColored(win::Color::Accent, title);
    win::WriteLine(L"");
}

void ConsoleUi::SetContextValue(const std::wstring& key, const std::wstring& value) {
    if (value.empty()) {
        contextValues_.erase(key);
        return;
    }
    contextValues_[key] = value;
}

void ConsoleUi::RenderStageFrame(const std::wstring& title) const {
    win::ClearScreen();
    win::WriteLineColored(win::Color::Accent, std::wstring(constants::kAppTitle));
    win::WriteLineColored(win::Color::Dim, L"帮助你将注入包写入体验服 Game 目录");
    win::WriteLine(L"");

    bool hasContext = false;
    for (const auto* key : kContextOrder) {
        const auto it = contextValues_.find(key);
        if (it != contextValues_.end()) {
            hasContext = true;
            break;
        }
    }

    if (hasContext) {
        win::WriteLineColored(win::Color::Dim, L"当前信息");
        win::WriteLineColored(win::Color::Dim, L"----------------------------------------");
        for (const auto* key : kContextOrder) {
            const auto it = contextValues_.find(key);
            if (it == contextValues_.end()) {
                continue;
            }
            win::WriteLine(BuildLabelValueLine(key, it->second, 12));
        }
        win::WriteLineColored(win::Color::Dim, L"----------------------------------------");
        win::WriteLine(L"");
    }

    win::WriteLineColored(win::Color::Accent, L"> " + title);
    win::WriteLine(L"");
}

std::wstring ConsoleUi::PromptLine(const std::wstring& prompt) const {
    win::Write(prompt);
    return win::ReadLine();
}

// BOT 卡密输入单独做成一个小面板，目的是提前说明写入范围和跳过方式。
std::wstring ConsoleUi::PromptBotKey() const {
    PromptTitle(L"输入 BOT 卡密");
    win::WriteLine(L"  将会写入本次成功注入的 user.ini 文件。");
    win::WriteLine(L"  如果暂时不需要，可以直接回车跳过。");
    return PromptLine(L"\n请输入 BOT 卡密：");
}

// 覆盖模式使用编号菜单，不再要求用户记忆英文模式名或快捷字母。
OverwriteMode ConsoleUi::PromptOverwriteMode() const {
    PromptTitle(L"选择文件冲突处理方式");
    win::WriteLine(L"  当目标目录中已存在同名文件时，请选择处理方式：");
    win::WriteLine(L"");
    win::WriteLine(L"  [1] 逐个确认");
    win::WriteLine(L"      每次遇到冲突时再询问你");
    win::WriteLine(L"");
    win::WriteLine(L"  [2] 全部覆盖");
    win::WriteLine(L"      遇到同名文件时直接替换");
    win::WriteLine(L"");
    win::WriteLine(L"  [3] 全部跳过");
    win::WriteLine(L"      保留原文件，不进行替换");
    while (true) {
        const std::wstring value = PromptLine(L"\n请输入编号：");
        if (value == L"1") return OverwriteMode::Ask;
        if (value == L"2") return OverwriteMode::Overwrite;
        if (value == L"3") return OverwriteMode::Skip;
        win::WriteLine(L"输入无效，请重新输入正确的编号。");
    }
}

bool ConsoleUi::ShouldWriteFile(OverwriteState& state, const std::filesystem::path& destinationPath) const {
    if (!std::filesystem::exists(destinationPath)) {
        return true;
    }
    if (state.mode == OverwriteMode::Overwrite) {
        return true;
    }
    if (state.mode == OverwriteMode::Skip) {
        return false;
    }
    if (state.escalation == AskEscalation::OverwriteAll) {
        return true;
    }
    if (state.escalation == AskEscalation::SkipAll) {
        return false;
    }

    // ASK 模式同时支持“当前文件单独决策”和“后续文件统一决策”，
    // 用编号菜单替代 Y/N/A/K，降低普通用户理解成本，也让交互风格保持一致。
    while (true) {
        PromptTitle(L"发现文件冲突");
        win::WriteLine(L"  以下文件已经存在：");
        win::WriteLine(L"  " + destinationPath.wstring());
        win::WriteLine(L"");
        win::WriteLine(L"请选择处理方式：");
        win::WriteLine(L"");
        win::WriteLine(L"  [1] 覆盖当前文件");
        win::WriteLine(L"  [2] 跳过当前文件");
        win::WriteLine(L"  [3] 覆盖后续所有冲突文件");
        win::WriteLine(L"  [4] 跳过后续所有冲突文件");
        const std::wstring value = PromptLine(L"\n请输入编号：");
        if (value == L"1") return true;
        if (value == L"2") return false;
        if (value == L"3") { state.escalation = AskEscalation::OverwriteAll; return true; }
        if (value == L"4") { state.escalation = AskEscalation::SkipAll; return false; }
        win::WriteLine(L"输入无效，请重新输入正确的编号。");
    }
}

std::optional<std::filesystem::path> ConsoleUi::PromptCandidateChoice(const std::vector<std::filesystem::path>& candidates, const std::wstring& title) const {
    if (candidates.empty()) {
        return std::nullopt;
    }
    if (candidates.size() == 1) {
        return candidates.front();
    }
    // 多个候选目录时统一进入列表选择界面。
    // 每个候选项都保留独立行距，避免路径较长时挤成难读的一团文本。
    PromptTitle(title);
    win::WriteLine(L"  已找到以下可用目录，请选择一个：");
    win::WriteLine(L"");
    for (size_t i = 0; i < candidates.size(); ++i) {
        win::WriteLine(L"  [" + std::to_wstring(i + 1) + L"] " + candidates[i].wstring());
        if (i == 0) {
            win::WriteLine(L"      推荐，可直接用于注入");
        } else {
            win::WriteLine(L"      可直接作为体验服目标目录");
        }
        win::WriteLine(L"");
    }
    win::WriteLine(L"  [0] 手动选择目录");

    while (true) {
        const std::wstring value = PromptLine(L"\n请输入编号：");
        try {
            const int idx = std::stoi(value);
            if (idx == 0) {
                // 空 path 是一个约定信号，表示切换到手动输入目录流程。
                return std::filesystem::path();
            }
            if (idx > 0 && static_cast<size_t>(idx) <= candidates.size()) {
                return candidates[idx - 1];
            }
        } catch (...) {
        }
        win::WriteLine(L"输入无效，请重新输入正确的编号。");
    }
}

} // namespace wg
