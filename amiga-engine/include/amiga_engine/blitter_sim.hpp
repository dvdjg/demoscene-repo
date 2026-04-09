#pragma once

#include <amiga_engine/blitter_queue.hpp>

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace amiga::engine {

/**
 * Tiny host-side planar framebuffer (1 bit per pixel, row-major bytes) for tests.
 */
class PlanarFramebuffer {
public:
  PlanarFramebuffer(std::uint16_t width, std::uint16_t height);

  [[nodiscard]] std::uint16_t width() const { return w_; }
  [[nodiscard]] std::uint16_t height() const { return h_; }

  [[nodiscard]] std::span<std::uint8_t> bytes() { return storage_; }
  [[nodiscard]] std::span<const std::uint8_t> bytes() const { return storage_; }

  void clear(std::uint8_t v);

private:
  std::uint16_t w_, h_;
  std::vector<std::uint8_t> storage_;
};

void run_blitter_queue(const BlitterQueue &q, PlanarFramebuffer &fb,
                       std::span<const std::uint8_t> source_plane);

} // namespace amiga::engine
