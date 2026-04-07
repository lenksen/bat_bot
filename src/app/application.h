#pragma once

#include "core/types.h"

namespace wg {

// 主流程入口声明。
// application.cpp 负责把启动参数、目标目录解析、归档写入、后处理、配置保存
// 和最终摘要输出串成一个完整的业务流程。
// 执行完整的 BOT 注入流程，并返回进程退出码。
int RunApplication(const RunOptions& options);

} // namespace wg
