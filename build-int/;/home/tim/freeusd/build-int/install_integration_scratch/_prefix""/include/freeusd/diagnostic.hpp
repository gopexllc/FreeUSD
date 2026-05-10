#pragma once

#include <string>
#include <vector>

#include "freeusd/export.hpp"
#include "freeusd/source_location.hpp"

namespace freeusd {

enum class DiagnosticSeverity {
  Note,
  Warning,
  Error,
};

struct Diagnostic {
  DiagnosticSeverity severity{DiagnosticSeverity::Error};
  std::string message;
  SourceLocation location;
};

using Diagnostics = std::vector<Diagnostic>;

FREEUSD_API const char* to_string(DiagnosticSeverity severity);
FREEUSD_API bool has_errors(const Diagnostics& diagnostics);

}  // namespace freeusd
