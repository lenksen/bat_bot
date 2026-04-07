#pragma once

#include <string>
#include <vector>

#include "core/context.h"
#include "core/types.h"

namespace wg::postprocess {

PostProcessResult Run(const Services& services, const std::vector<std::filesystem::path>& manifest, const std::wstring& botKey);

} // namespace wg::postprocess
