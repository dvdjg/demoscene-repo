#include <amiga_engine/blitter_sim.hpp>

#include <algorithm>

namespace amiga::engine {

PlanarFramebuffer::PlanarFramebuffer(std::uint16_t width, std::uint16_t height)
    : w_(width), h_(height) {
  const std::size_t row_bytes = (static_cast<std::size_t>(w_) + 7) / 8;
  storage_.assign(row_bytes * h_, 0);
}

void PlanarFramebuffer::clear(std::uint8_t v) { std::fill(storage_.begin(), storage_.end(), v); }

static std::size_t row_bytes(std::uint16_t w) { return (static_cast<std::size_t>(w) + 7) / 8; }

static bool bit_at(std::span<const std::uint8_t> plane, std::uint16_t w, std::uint16_t x,
                   std::uint16_t y) {
  const std::size_t rb = row_bytes(w);
  const std::size_t idx = y * rb + x / 8;
  const unsigned bit = 7 - (x % 8);
  return (plane[idx] >> bit) & 1;
}

static void set_bit(std::span<std::uint8_t> plane, std::uint16_t w, std::uint16_t x, std::uint16_t y,
                    bool on) {
  const std::size_t rb = row_bytes(w);
  const std::size_t idx = y * rb + x / 8;
  const unsigned bit = 7 - (x % 8);
  if (on)
    plane[idx] |= static_cast<std::uint8_t>(1u << bit);
  else
    plane[idx] &= static_cast<std::uint8_t>(~(1u << bit));
}

void run_blitter_queue(const BlitterQueue &q, PlanarFramebuffer &fb,
                       std::span<const std::uint8_t> source_plane) {
  const std::uint16_t fw = fb.width();
  const std::uint16_t fh = fb.height();
  for (const BlitOp &op : q.ops()) {
    if (op.kind == BlitOpKind::FillRect) {
      for (std::uint16_t yy = 0; yy < op.dst.h; ++yy) {
        const std::uint16_t y = op.dst.y + yy;
        if (y >= fh)
          break;
        for (std::uint16_t xx = 0; xx < op.dst.w; ++xx) {
          const std::uint16_t x = op.dst.x + xx;
          if (x >= fw)
            break;
          set_bit(fb.bytes(), fw, x, y, (op.fill_value & 1u) != 0);
        }
      }
    } else {
      for (std::uint16_t yy = 0; yy < op.dst.h; ++yy) {
        const std::uint16_t sy = op.src_y + yy;
        const std::uint16_t dy = op.dst.y + yy;
        if (dy >= fh)
          break;
        for (std::uint16_t xx = 0; xx < op.dst.w; ++xx) {
          const std::uint16_t sx = op.src_x + xx;
          const std::uint16_t dx = op.dst.x + xx;
          if (dx >= fw)
            break;
          if (!source_plane.empty())
            set_bit(fb.bytes(), fw, dx, dy,
                    bit_at(source_plane, fw, sx % fw, sy % fh));
        }
      }
    }
  }
}

} // namespace amiga::engine
