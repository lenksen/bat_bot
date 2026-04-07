#include "ui/console_ui.h"

#include <string>

#include "core/constants.h"
#include "io/logger.h"
#include "win/console.h"

namespace wg {

ConsoleUi::ConsoleUi(Logger& logger) : logger_(logger) {}

void ConsoleUi::ShowBanner() const {
    win::WriteLine(L"============================================================");
    win::WriteLine(std::wstring(constants::kBannerTitle));
    win::WriteLine(L"============================================================");
}

void ConsoleUi::Step(const std::wstring& message) const {
    win::WriteLine(L"");
    win::WriteLine(L"==> " + message);
    logger_.Info(message);
}

void ConsoleUi::Info(const std::wstring& message) const { win::WriteLine(L"    " + message); }
void ConsoleUi::Warn(const std::wstring& message) const { win::WriteLine(L"    " + message); logger_.Warn(message); }
void ConsoleUi::Error(const std::wstring& message) const { win::WriteLine(L"[ERROR] " + message); }

std::wstring ConsoleUi::PromptLine(const std::wstring& prompt) const {
    win::Write(prompt);
    return win::ReadLine();
}

OverwriteMode ConsoleUi::PromptOverwriteMode() const {
    win::WriteLine(L"");
    win::WriteLine(L"请选择覆盖模式：");
    win::WriteLine(L"[1] ASK       冲突时逐个询问（可切换为全部覆盖/全部跳过）");
    win::WriteLine(L"[2] OVERWRITE 全部覆盖");
    win::WriteLine(L"[3] SKIP      全部跳过已存在文件");
    while (true) {
        const std::wstring value = PromptLine(L"输入 1 / 2 / 3: ");
        if (value == L"1") return OverwriteMode::Ask;
        if (value == L"2") return OverwriteMode::Overwrite;
        if (value == L"3") return OverwriteMode::Skip;
        win::WriteLine(L"输入无效，请重新输入。");
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

    while (true) {
        win::WriteLine(L"文件已存在: " + destinationPath.wstring());
        const std::wstring value = PromptLine(L"Y=覆盖 N=跳过 A=后续全部覆盖 K=后续全部跳过: ");
        if (value == L"Y" || value == L"y") return true;
        if (value == L"N" || value == L"n") return false;
        if (value == L"A" || value == L"a") { state.escalation = AskEscalation::OverwriteAll; return true; }
        if (value == L"K" || value == L"k") { state.escalation = AskEscalation::SkipAll; return false; }
        win::WriteLine(L"输入无效，请重新输入。");
    }
}

std::optional<std::filesystem::path> ConsoleUi::PromptCandidateChoice(const std::vector<std::filesystem::path>& candidates, const std::wstring& title) const {
    if (candidates.empty()) {
        return std::nullopt;
    }
    if (candidates.size() == 1) {
        return candidates.front();
    }
    win::WriteLine(L"");
    win::WriteLine(title);
    for (size_t i = 0; i < candidates.size(); ++i) {
        win::WriteLine(L"[" + std::to_wstring(i + 1) + L"] " + candidates[i].wstring());
    }
    win::WriteLine(L"[0] 手动选择目录");

    while (true) {
        const std::wstring value = PromptLine(L"请输入编号: ");
        try {
            const int idx = std::stoi(value);
            if (idx == 0) {
                return std::filesystem::path();
            }
            if (idx > 0 && static_cast<size_t>(idx) <= candidates.size()) {
                return candidates[idx - 1];
            }
        } catch (...) {
        }
        win::WriteLine(L"输入无效，请重新输入。");
    }
}

} // namespace wg
