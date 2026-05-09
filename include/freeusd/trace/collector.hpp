#pragma once

namespace freeusd::trace {

/// No-op tracing hook (TraceCollector-shaped).
class Collector {
 public:
  static Collector& Get() noexcept {
    static Collector c;
    return c;
  }
  void Push(const char*) noexcept {}
  void Pop() noexcept {}
};

}  // namespace freeusd::trace

#define FREEUSD_TRACE_FUNCTION() ((void)0)
