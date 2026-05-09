#include "freeusd/diagnostic.hpp"

namespace freeusd {

const char* to_string(DiagnosticSeverity severity) {
  switch (severity) {
    case DiagnosticSeverity::Note:
      return "note";
    case DiagnosticSeverity::Warning:
      return "warning";
    case DiagnosticSeverity::Error:
      return "error";
  }
  return "error";
}

bool has_errors(const Diagnostics& diagnostics) {
  for (const Diagnostic& diagnostic : diagnostics) {
    if (diagnostic.severity == DiagnosticSeverity::Error) {
      return true;
    }
  }
  return false;
}

}  // namespace freeusd
