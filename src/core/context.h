#pragma once

#include "core/types.h"

namespace wg {

class Logger;
class ConsoleUi;

// 聚合一次运行中会频繁传递的公共服务对象。
// 这样业务函数之间只需要传递一个 Services，避免参数列表不断膨胀。
struct Services {
    AppContext app;
    Logger* logger = nullptr;
    ConsoleUi* ui = nullptr;
};

} // namespace wg
