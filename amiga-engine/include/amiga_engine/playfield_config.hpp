#pragma once

#include <cstdint>

namespace amiga::engine {

/// Mirrors MODE_* bits from playfield.h (OCS low-res baseline).
enum class PlayfieldFlags : std::uint16_t {
  None = 0,
  Hires = 1 << 15,
  DualPf = 1 << 10,
  Ham = 1 << 11,
  Lace = 1 << 2,
};

[[nodiscard]] constexpr PlayfieldFlags operator|(PlayfieldFlags a, PlayfieldFlags b) noexcept {
  return static_cast<PlayfieldFlags>(static_cast<std::uint16_t>(a) |
                                     static_cast<std::uint16_t>(b));
}

[[nodiscard]] constexpr std::uint16_t to_u16(PlayfieldFlags f) noexcept {
  return static_cast<std::uint16_t>(f);
}

struct PlayfieldConfig {
  PlayfieldFlags mode{PlayfieldFlags::None};
  std::uint16_t depth{4};
  std::uint16_t diw_x{};
  std::uint16_t diw_y{};
  std::uint16_t width{320};
  std::uint16_t height{256};
};

} // namespace amiga::engine
