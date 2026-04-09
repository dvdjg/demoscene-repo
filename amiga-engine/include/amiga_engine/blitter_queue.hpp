#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace amiga::engine {

enum class BlitOpKind : std::uint8_t { CopyRect, FillRect };

struct BlitRect {
  std::uint16_t x{}, y{}, w{}, h{};
};

struct BlitOp {
  BlitOpKind kind{BlitOpKind::CopyRect};
  BlitRect dst{};
  std::uint32_t fill_value{0};
  /* Copy: source offset into simulated plane (host test harness). */
  std::uint16_t src_x{}, src_y{};
};

/**
 * Retained blitter work for one frame. On Amiga this would flush to hw registers
 * (see lib/libblit); on host we run BlitterSimulator.
 */
class BlitterQueue {
public:
  void clear() { ops_.clear(); }

  void enqueue(const BlitOp &op) { ops_.push_back(op); }

  [[nodiscard]] std::span<const BlitOp> ops() const { return ops_; }

  [[nodiscard]] std::size_t count() const { return ops_.size(); }

  /// Returns false if budget exceeded (drops extra ops).
  bool set_budget(std::size_t max_ops);

private:
  std::vector<BlitOp> ops_;
};

} // namespace amiga::engine
