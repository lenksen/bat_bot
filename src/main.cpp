#include <filesystem>

#include "app/application.h"

int wmain(int argc, wchar_t* argv[]) {
    wg::RunOptions options;
    if (argc > 1 && argv[1] && argv[1][0] != L'\0') {
        options.initialArchivePath = std::filesystem::path(argv[1]);
    }
    return wg::RunApplication(options);
}
