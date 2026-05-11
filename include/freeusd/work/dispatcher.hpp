#pragma once

#include <atomic>
#include <cstdint>
#include <functional>

namespace freeusd::work {

/// Minimal serial dispatcher (WorkDispatcher-shaped; no thread pool yet).
class Dispatcher {
 public:
  static Dispatcher& Get() noexcept;
  void Run(const std::function<void()>& job);
  std::uint64_t CompletedJobs() const noexcept { return completed_jobs_.load(); }
  void ResetCompletedJobs() noexcept { completed_jobs_.store(0); }

 private:
  inline static std::atomic<std::uint64_t> completed_jobs_{0};
};

inline Dispatcher& Dispatcher::Get() noexcept {
  static Dispatcher d;
  return d;
}

inline void Dispatcher::Run(const std::function<void()>& job) {
  if (job) {
    job();
    completed_jobs_.fetch_add(1);
  }
}

}  // namespace freeusd::work
