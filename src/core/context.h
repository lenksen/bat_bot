#pragma once

#include "core/types.h"

namespace wg {

class Logger;
class ConsoleUi;

struct Services {
    AppContext app;
    Logger* logger = nullptr;
    ConsoleUi* ui = nullptr;
};

} // namespace wg
