#pragma once

#include <functional>

namespace freeusd::work {

/// Minimal serial dispatcher (WorkDispatcher-shaped; no thread pool yet).
class Dispatcher {
 public:
  static Dispatcher& Get() noexcept;
  void Run(const std::function<void()>& job);
};

inline Dispatcher& Dispatcher::Get() noexcept {
  static Dispatcher d;
  return d;
}

inline void Dispatcher::Run(const std::function<void()>& job) {
  if (job) {
    job();
  }
}

}  // namespace freeusd::work
