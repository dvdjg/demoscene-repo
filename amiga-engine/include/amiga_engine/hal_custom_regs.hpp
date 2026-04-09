#pragma once

#include <cstdint>

namespace amiga::engine::hal {

/// Byte offsets into `struct Custom` (same convention as CSREG in copper.h).
enum class CustomReg : std::uint16_t {
  color0 = 0x180,
  color1 = 0x182,
  bplcon0 = 0x100,
  bplcon1 = 0x102,
  bplcon2 = 0x104,
  dmacon = 0x096,
  copcon = 0x02c,
  cop1lc = 0x080,
  cop2lc = 0x084,
};

[[nodiscard]] constexpr std::uint16_t custom_reg_u16(CustomReg r) noexcept {
  return static_cast<std::uint16_t>(r);
}

} // namespace amiga::engine::hal
