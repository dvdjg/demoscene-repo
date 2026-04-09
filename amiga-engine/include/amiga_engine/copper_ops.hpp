#pragma once

#include <cstddef>
#include <cstdint>
#include <variant>
#include <vector>

namespace amiga::engine {

/**
 * Semantic copper operations (host-testable). Maps to Amiga copper instructions.
 */
struct CopperMove {
  std::uint16_t reg_offset{};
  std::uint16_t value{};
};

struct CopperWait {
  std::uint8_t vp{};
  /// Horizontal position in color clocks (before >>1 inside copper).
  std::uint16_t hp{};
};

struct CopperEnd {};

using CopperInstruction = std::variant<CopperMove, CopperWait, CopperEnd>;

class CopperProgramBuilder {
public:
  void move(std::uint16_t reg, std::uint16_t value) {
    ops_.push_back(CopperMove{reg, value});
  }

  void wait(std::uint8_t vp, std::uint16_t hp) { ops_.push_back(CopperWait{vp, hp}); }

  void end() { ops_.push_back(CopperEnd{}); }

  [[nodiscard]] const std::vector<CopperInstruction> &instructions() const { return ops_; }

  /**
   * Encode to 16-bit words (host native order; matches logical MOVE/WAIT layout
   * used by the framework's CopIns helpers).
   */
  [[nodiscard]] std::vector<std::uint16_t> encode_words() const;

private:
  std::vector<CopperInstruction> ops_;
};

} // namespace amiga::engine
