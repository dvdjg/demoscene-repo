#pragma once

#include <cstdint>

namespace amiga::engine {

/// Feature flags for A1200 path (optional backend; OCS remains default).
struct AgaFeatures {
  bool enabled{false};
  std::uint8_t extra_planes{0};
};

[[nodiscard]] inline AgaFeatures detect_aga_runtime_stub() {
  return AgaFeatures{};
}

} // namespace amiga::engine
