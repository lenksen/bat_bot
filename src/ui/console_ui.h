#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

#include "core/types.h"

namespace wg {

class Logger;

class ConsoleUi {
public:
    explicit ConsoleUi(Logger& logger);

    void ShowBanner() const;
    void Step(const std::wstring& message) const;
    void Info(const std::wstring& message) const;
    void Warn(const std::wstring& message) const;
    void Error(const std::wstring& message) const;

    std::wstring PromptLine(const std::wstring& prompt) const;
    OverwriteMode PromptOverwriteMode() const;
    bool ShouldWriteFile(OverwriteState& state, const std::filesystem::path& destinationPath) const;
    std::optional<std::filesystem::path> PromptCandidateChoice(const std::vector<std::filesystem::path>& candidates, const std::wstring& title) const;

private:
    Logger& logger_;
};

} // namespace wg
