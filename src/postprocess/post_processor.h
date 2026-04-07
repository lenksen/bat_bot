#pragma once

#include <string>
#include <vector>

#include "core/context.h"
#include "core/types.h"

namespace wg::postprocess {

// 注入后处理层。
// 这里的处理严格以 manifest 为边界，只处理本次运行真实写入的文件，
// 不会回头扫描整个 Game 目录。
// 对本次 manifest 中记录的文件执行后处理。
// 注意：不会重新扫描整个 Game 目录。
PostProcessResult Run(const Services& services, const std::vector<std::filesystem::path>& manifest, const std::wstring& botKey);

} // namespace wg::postprocess
