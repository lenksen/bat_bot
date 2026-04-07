#include "ui/dialogs.h"

#include <Windows.h>
#include <shobjidl.h>

#include <stdexcept>

namespace wg::ui {

namespace {

class ComInit {
public:
    ComInit() : hr_(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE)) {}
    ~ComInit() { if (SUCCEEDED(hr_)) CoUninitialize(); }
    HRESULT hr() const { return hr_; }
private:
    HRESULT hr_;
};

std::optional<std::filesystem::path> ShowDialog(bool pickFolder, const std::filesystem::path& initialDirectory) {
    ComInit com;
    if (FAILED(com.hr())) {
        return std::nullopt;
    }

    IFileDialog* dialog = nullptr;
    if (FAILED(CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&dialog)))) {
        return std::nullopt;
    }

    DWORD options = 0;
    dialog->GetOptions(&options);
    options |= FOS_FORCEFILESYSTEM | FOS_PATHMUSTEXIST;
    if (pickFolder) {
        options |= FOS_PICKFOLDERS;
    }
    dialog->SetOptions(options);

    if (!pickFolder) {
        COMDLG_FILTERSPEC spec[] = {{L"压缩包 (*.zip;*.rar)", L"*.zip;*.rar"}};
        dialog->SetFileTypes(1, spec);
        dialog->SetTitle(L"请选择BOT注入包");
    } else {
        dialog->SetTitle(L"请选择体验服目录或其 Game 目录");
    }

    if (!initialDirectory.empty() && std::filesystem::exists(initialDirectory)) {
        IShellItem* folder = nullptr;
        if (SUCCEEDED(SHCreateItemFromParsingName(initialDirectory.c_str(), nullptr, IID_PPV_ARGS(&folder)))) {
            dialog->SetDefaultFolder(folder);
            folder->Release();
        }
    }

    const HRESULT showHr = dialog->Show(nullptr);
    if (FAILED(showHr)) {
        dialog->Release();
        return std::nullopt;
    }

    IShellItem* result = nullptr;
    if (FAILED(dialog->GetResult(&result))) {
        dialog->Release();
        return std::nullopt;
    }

    PWSTR raw = nullptr;
    if (FAILED(result->GetDisplayName(SIGDN_FILESYSPATH, &raw))) {
        result->Release();
        dialog->Release();
        return std::nullopt;
    }

    std::filesystem::path path(raw);
    CoTaskMemFree(raw);
    result->Release();
    dialog->Release();
    return path;
}

} // namespace

std::optional<std::filesystem::path> PickArchiveFile(const std::filesystem::path& initialDirectory) {
    return ShowDialog(false, initialDirectory);
}

std::optional<std::filesystem::path> PickFolder(const std::filesystem::path& initialDirectory) {
    return ShowDialog(true, initialDirectory);
}

} // namespace wg::ui
