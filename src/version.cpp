#include "freeusd/version.hpp"

#include <cstdint>
#include <string>

namespace freeusd {

std::string version_string() {
  return std::to_string(FREEUSD_VERSION_MAJOR) + "." + std::to_string(FREEUSD_VERSION_MINOR) + "." +
         std::to_string(FREEUSD_VERSION_PATCH);
}

void version_parts(std::uint32_t& major, std::uint32_t& minor, std::uint32_t& patch) {
  major = FREEUSD_VERSION_MAJOR;
  minor = FREEUSD_VERSION_MINOR;
  patch = FREEUSD_VERSION_PATCH;
}

}  // namespace freeusd
