#pragma once

#include <filesystem>

#include "core/context.h"
#include "core/types.h"

namespace wg::archive {

std::filesystem::path ResolveArchiveInput(const Services& services, const std::optional<std::filesystem::path>& initialArchive);
ExtractResult ExtractGameArchiveSubset(const Services& services, const std::filesystem::path& archivePath, const std::filesystem::path& targetRoot, OverwriteMode overwriteMode, const std::filesystem::path& manifestFile);

} // namespace wg::archive
