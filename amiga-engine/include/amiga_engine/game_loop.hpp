#pragma once

#include <chrono>
#include <functional>

namespace amiga::engine {

using Clock = std::chrono::steady_clock;

/**
 * Host-side fixed-step loop sketch; on Amiga, wire to CIA / vblank instead.
 */
class GameLoop {
public:
  using TickFn = std::function<void(float dt)>;

  explicit GameLoop(float fixed_dt_seconds, TickFn tick) : dt_(fixed_dt_seconds), tick_(std::move(tick)) {}

  void run_steps(int n) {
    for (int i = 0; i < n; ++i)
      tick_(dt_);
  }

private:
  float dt_;
  TickFn tick_;
};

} // namespace amiga::engine
