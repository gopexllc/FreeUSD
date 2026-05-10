#pragma once

#include <cstdint>
#include <string>

#include "freeusd/export.hpp"

namespace freeusd {

/// Project version string, e.g. "0.1.0".
FREEUSD_API std::string version_string();

/// (major, minor, patch) from the build.
FREEUSD_API void version_parts(std::uint32_t& major, std::uint32_t& minor, std::uint32_t& patch);

}  // namespace freeusd
