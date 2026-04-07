#include <filesystem>

#include "app/application.h"

// Windows 下使用宽字符入口函数启动程序。
// 这样命令行参数里的中文路径可以从进程入口开始就保持为宽字符，
// 避免依赖 ANSI 代码页转换带来的乱码或路径识别失败。
int wmain(int argc, wchar_t* argv[]) {
    wg::RunOptions options;
    if (argc > 1 && argv[1] && argv[1][0] != L'\0') {
        options.initialArchivePath = std::filesystem::path(argv[1]);
    }
    return wg::RunApplication(options);
}
