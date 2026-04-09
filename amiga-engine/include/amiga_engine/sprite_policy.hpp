#pragma once

#include <amiga_engine/blitter_queue.hpp>

#include <cstdint>
#include <memory>
#include <string_view>

namespace amiga::engine {

struct Transform2 {
  float x{0}, y{0};
};

/**
 * Strategy for how a logical sprite is realized (hardware sprite, BOB, soft, copper bar…).
 */
class ISpritePolicy {
public:
  virtual ~ISpritePolicy() = default;
  virtual void enqueue_draw(BlitterQueue &q, Transform2 t) const = 0;
  [[nodiscard]] virtual std::string_view name() const = 0;
};

/// Placeholder: would program SPRxPOS/SPRxCTL and attach chip data.
class HardwareSpritePolicy final : public ISpritePolicy {
public:
  explicit HardwareSpritePolicy(int channel) : channel_(channel) {}
  void enqueue_draw(BlitterQueue &q, Transform2 t) const override;
  [[nodiscard]] std::string_view name() const override { return "hw_sprite"; }

private:
  int channel_;
};

/// BOB path: expand to masked blits (host sim ignores mask).
class BobSpritePolicy final : public ISpritePolicy {
public:
  void enqueue_draw(BlitterQueue &q, Transform2 t) const override;
  [[nodiscard]] std::string_view name() const override { return "bob"; }
};

/// CPU-maintained bitmap + optional copper splits for colors.
class SoftSpritePolicy final : public ISpritePolicy {
public:
  void enqueue_draw(BlitterQueue &q, Transform2 t) const override;
  [[nodiscard]] std::string_view name() const override { return "soft"; }
};

} // namespace amiga::engine
