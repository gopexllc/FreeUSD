#pragma once

namespace freeusd::trace {

/// Lightweight tracing hook (TraceCollector-shaped): stack depth is thread-local.
class Collector {
 public:
  static Collector& Get() noexcept {
    static Collector c;
    return c;
  }
  void Push(const char*) noexcept { ++stack_depth_; }
  void Pop() noexcept {
    if (stack_depth_ > 0) {
      --stack_depth_;
    }
  }
  unsigned StackDepth() const noexcept { return stack_depth_; }
  void Reset() noexcept { stack_depth_ = 0; }

 private:
  inline static thread_local unsigned stack_depth_{0};
};

}  // namespace freeusd::trace

#define FREEUSD_TRACE_FUNCTION() ((void)0)
