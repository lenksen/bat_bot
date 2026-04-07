#include "fs/target_resolver.h"

#include <Windows.h>

#include <algorithm>
#include <chrono>
#include <queue>
#include <stack>

#include "core/constants.h"
#include "core/errors.h"
#include "fs/path_utils.h"
#include "io/config_store.h"
#include "io/logger.h"
#include "ui/console_ui.h"
#include "ui/dialogs.h"
#include "win/encoding.h"

namespace wg::fs {

namespace {

// 枚举本机固定磁盘，并把非系统盘排在前面。
// 体验服更常安装在非系统盘，优先搜索这些磁盘通常更高效。
std::vector<DriveObject> GetFixedDriveObjects() {
    std::vector<DriveObject> out;
    const DWORD mask = GetLogicalDrives();
    wchar_t systemRootBuffer[MAX_PATH] = {};
    GetEnvironmentVariableW(L"SystemDrive", systemRootBuffer, MAX_PATH);
    std::wstring systemDrive = systemRootBuffer[0] ? systemRootBuffer : L"C:";

    for (wchar_t letter = L'A'; letter <= L'Z'; ++letter) {
        const int bit = letter - L'A';
        if ((mask & (1u << bit)) == 0) continue;
        std::wstring root = std::wstring(1, letter) + L":\\";
        if (GetDriveTypeW(root.c_str()) != DRIVE_FIXED) continue;
        out.push_back({std::filesystem::path(root).root_name(), std::filesystem::path(root), _wcsicmp(root.substr(0, 2).c_str(), systemDrive.c_str()) == 0});
    }

    std::sort(out.begin(), out.end(), [](const DriveObject& a, const DriveObject& b) {
        if (a.isSystem != b.isSystem) return !a.isSystem && b.isSystem;
        return a.root.wstring() < b.root.wstring();
    });
    return out;
}

// 先检查常见安装路径。
// 这是自动搜索中成本最低的一步，应该优先于递归遍历。
std::vector<std::filesystem::path> GetCommonCandidateRoots(const std::vector<DriveObject>& drives) {
    std::vector<std::filesystem::path> candidates;
    for (const auto& drive : drives) {
        const std::vector<std::wstring> rels = drive.isSystem
            ? std::vector<std::wstring>{L"WeGameApps\\英雄联盟体验服", L"Program Files\\WeGameApps\\英雄联盟体验服", L"Program Files (x86)\\WeGameApps\\英雄联盟体验服"}
            : std::vector<std::wstring>{L"WeGameApps\\英雄联盟体验服", L"Tencent\\WeGameApps\\英雄联盟体验服", L"Games\\WeGameApps\\英雄联盟体验服"};
        for (const auto& rel : rels) {
            candidates.push_back(drive.rootPath / rel);
        }
    }
    return UniqueOrdered(candidates);
}

// 安全地枚举一个目录下的子目录。
// 权限不足、损坏路径或其他枚举异常都只影响当前分支，不中断整体搜索。
std::vector<std::filesystem::path> EnumerateDirectoriesSafe(const std::filesystem::path& path) {
    std::vector<std::filesystem::path> out;
    std::error_code ec;
    for (const auto& entry : std::filesystem::directory_iterator(path, ec)) {
        if (ec) break;
        if (entry.is_directory(ec) && !ec) {
            out.push_back(entry.path());
        }
    }
    return out;
}

// 在有限深度内优先寻找 WeGameApps 根目录。
// 找到后，后续更深层的搜索就可以聚焦在这些目录下面，避免整盘暴力扫描。
std::vector<std::filesystem::path> FindWeGameAppsShallow(const std::filesystem::path& root, int maxDepth) {
    std::vector<std::filesystem::path> found;
    std::queue<std::pair<std::filesystem::path, int>> q;
    q.push({root, 0});
    while (!q.empty()) {
        auto [current, depth] = q.front();
        q.pop();
        if (depth >= maxDepth) continue;
        for (const auto& child : EnumerateDirectoriesSafe(current)) {
            if (_wcsicmp(child.filename().c_str(), L"WeGameApps") == 0) {
                found.push_back(child);
            }
            q.push({child, depth + 1});
        }
    }
    return UniqueOrdered(found);
}

// 在已发现的 WeGameApps 根目录下继续做有限深度搜索。
// 这是标准子路径直查失败后的第二层自动搜索策略。
std::vector<std::filesystem::path> FindLolLimitedDepth(const std::filesystem::path& root, int maxDepth) {
    std::vector<std::filesystem::path> found;
    std::queue<std::pair<std::filesystem::path, int>> q;
    q.push({root, 0});
    while (!q.empty()) {
        auto [current, depth] = q.front();
        q.pop();
        if (depth >= maxDepth) continue;
        for (const auto& child : EnumerateDirectoriesSafe(current)) {
            if (_wcsicmp(child.filename().c_str(), std::wstring(constants::kGameName).c_str()) == 0) {
                if (auto valid = ValidateTargetRoot(child)) found.push_back(*valid);
            }
            q.push({child, depth + 1});
        }
    }
    return UniqueOrdered(found);
}

// 在 WeGameApps 范围内执行最终深度兜底搜索。
// 这一阶段开销最大，因此有意放到最后执行。
std::vector<std::filesystem::path> FindLolDeepFallback(const std::filesystem::path& root) {
    std::vector<std::filesystem::path> found;
    std::stack<std::filesystem::path> st;
    for (const auto& child : EnumerateDirectoriesSafe(root)) st.push(child);
    while (!st.empty()) {
        const auto current = st.top();
        st.pop();
        if (_wcsicmp(current.filename().c_str(), std::wstring(constants::kGameName).c_str()) == 0) {
            if (auto valid = ValidateTargetRoot(current)) found.push_back(*valid);
        }
        for (const auto& child : EnumerateDirectoriesSafe(current)) st.push(child);
    }
    return UniqueOrdered(found);
}

// 手动输入路径时仍然复用自动搜索使用的同一套合法性校验规则。
std::optional<std::filesystem::path> PickManualTarget(const Services& services, const std::filesystem::path& initial) {
    if (auto picked = ui::PickFolder(initial); picked.has_value()) {
        return ValidateTargetRoot(*picked);
    }
    const std::wstring raw = services.ui->PromptLine(L"请输入体验服目标目录路径: ");
    if (raw.empty()) return std::nullopt;
    return ValidateTargetRoot(raw);
}

// 返回空 path 不是失败，而是一个约定信号：
// 表示用户在候选列表界面选择了“改为手动选择目录”。
std::filesystem::path ResolveFromCandidates(const Services& services, const std::vector<std::filesystem::path>& candidates, const std::wstring& title, const std::wstring& source, const std::filesystem::path& initial) {
    auto choice = services.ui->PromptCandidateChoice(candidates, title);
    if (!choice.has_value()) return {};
    if (choice->empty()) {
        if (auto manual = PickManualTarget(services, initial)) {
            services.logger->Info(L"目标搜索完成，来源=" + source + L"，路径=" + manual->wstring());
            return *manual;
        }
        return {};
    }
    services.logger->Info(L"目标搜索完成，来源=" + source + L"，路径=" + choice->wstring());
    return *choice;
}

} // namespace

void CleanupOldWorkDirs(const std::filesystem::path& root, const std::filesystem::path& current) {
    std::error_code ec;
    if (!std::filesystem::is_directory(root, ec)) return;
    // 工作目录只属于临时运行产物。
    // 这里只保留当前运行和较新的历史目录，过旧目录做 best-effort 清理。
    const auto cutoff = std::filesystem::file_time_type::clock::now() - std::chrono::hours(24);
    for (const auto& entry : std::filesystem::directory_iterator(root, ec)) {
        if (ec) break;
        if (!entry.is_directory(ec) || ec) continue;
        if (entry.path() == current) continue;
        const auto last = entry.last_write_time(ec);
        if (!ec && last < cutoff) {
            std::filesystem::remove_all(entry.path(), ec);
        }
    }
}

std::filesystem::path ResolveTargetRoot(const Services& services) {
    services.ui->Step(L"检查 config.ini 中记录的上次目标目录");
    const ConfigValues config = ConfigStore::Load(services.app.configFile);
    if (config.lastPath.has_value()) {
        if (auto valid = ValidateTargetRoot(*config.lastPath)) {
            services.ui->Info(valid->wstring());
            services.logger->Info(L"目标搜索完成，来源=config.ini，路径=" + valid->wstring());
            return *valid;
        }
        services.ui->Warn(L"config.ini 中的 last_path 已失效: " + config.lastPath->wstring());
    } else {
        services.ui->Info(L"last_path 为空，继续自动搜索体验服目录。");
    }

    const auto drives = GetFixedDriveObjects();
    if (drives.empty()) {
        throw AppError("未检测到可用的固定磁盘。");
    }

    // 搜索顺序是刻意设计的：
    // 优先使用命中率高且成本低的检查，再逐步进入更重的递归搜索。
    services.ui->Step(L"检查常见体验服路径");
    std::vector<std::filesystem::path> commonHits;
    for (const auto& candidate : GetCommonCandidateRoots(drives)) {
        if (auto valid = ValidateTargetRoot(candidate)) commonHits.push_back(*valid);
    }
    commonHits = UniqueOrdered(commonHits);
    if (!commonHits.empty()) {
        auto resolved = ResolveFromCandidates(services, commonHits, L"发现以下体验服目录候选：", L"common-path", config.lastPath.value_or(services.app.exeDir));
        if (!resolved.empty()) return resolved;
    }

    std::vector<DriveObject> nonSystemDrives;
    for (const auto& drive : drives) {
        if (!drive.isSystem) nonSystemDrives.push_back(drive);
    }
    if (nonSystemDrives.empty()) {
        services.ui->Warn(L"未发现非系统盘，将跳过体验服目录浅搜索。");
    } else {
        services.ui->Step(L"自动搜索非系统盘中的体验服目录");
        std::vector<std::filesystem::path> weGameRoots;
        for (const auto& drive : nonSystemDrives) {
            services.ui->Info(L"正在搜索 " + drive.rootPath.wstring());
            auto found = FindWeGameAppsShallow(drive.rootPath, 3);
            weGameRoots.insert(weGameRoots.end(), found.begin(), found.end());
        }
        weGameRoots = UniqueOrdered(weGameRoots);

        if (!weGameRoots.empty()) {
            // 一旦已知 WeGameApps 根目录，就优先检查标准子路径，
            // 因为这是最符合常见安装布局的命中方式。
            services.ui->Step(L"优先检查已找到路径下的英雄联盟体验服目录");
            std::vector<std::filesystem::path> directHits;
            for (const auto& root : weGameRoots) {
                if (auto valid = ValidateTargetRoot(root / constants::kGameName)) directHits.push_back(*valid);
            }
            directHits = UniqueOrdered(directHits);
            if (!directHits.empty()) {
                auto resolved = ResolveFromCandidates(services, directHits, L"发现以下可直接使用的体验服目录：", L"direct-under-wegameapps", config.lastPath.value_or(services.app.exeDir));
                if (!resolved.empty()) return resolved;
            }

            services.ui->Step(L"执行有限深度搜索");
            std::vector<std::filesystem::path> limitedHits;
            for (const auto& root : weGameRoots) {
                auto found = FindLolLimitedDepth(root, 2);
                limitedHits.insert(limitedHits.end(), found.begin(), found.end());
            }
            limitedHits = UniqueOrdered(limitedHits);
            if (!limitedHits.empty()) {
                auto resolved = ResolveFromCandidates(services, limitedHits, L"发现以下体验服目录候选：", L"limited-depth", config.lastPath.value_or(services.app.exeDir));
                if (!resolved.empty()) return resolved;
            }

            services.ui->Step(L"执行深度兜底搜索");
            std::vector<std::filesystem::path> deepHits;
            for (const auto& root : weGameRoots) {
                auto found = FindLolDeepFallback(root);
                deepHits.insert(deepHits.end(), found.begin(), found.end());
            }
            deepHits = UniqueOrdered(deepHits);
            if (!deepHits.empty()) {
                auto resolved = ResolveFromCandidates(services, deepHits, L"发现以下深度搜索结果：", L"deep-fallback", config.lastPath.value_or(services.app.exeDir));
                if (!resolved.empty()) return resolved;
            }
        }
    }

    // 手动选择是所有自动搜索策略都失败后的最后兜底方案。
    services.ui->Step(L"未自动找到体验服目录，请手动选择");
    if (auto manual = PickManualTarget(services, config.lastPath.value_or(services.app.exeDir))) {
        services.logger->Info(L"目标搜索完成，来源=manual-final，路径=" + manual->wstring());
        return *manual;
    }
    throw AppError("未选择有效的体验服目标目录。");
}

} // namespace wg::fs
