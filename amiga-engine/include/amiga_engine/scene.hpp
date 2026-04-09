#pragma once

#include <amiga_engine/blitter_queue.hpp>
#include <amiga_engine/playfield_config.hpp>
#include <amiga_engine/sprite_policy.hpp>

#include <cstdint>
#include <memory>
#include <vector>

namespace amiga::engine {

using EntityId = std::uint32_t;

enum class LayerId : std::uint8_t {
  Background = 0,
  PlayfieldA,
  PlayfieldB,
  Bobs,
  Overlay,
  CopperFx,
};

struct DrawableEntity {
  EntityId id{};
  LayerId layer{LayerId::Bobs};
  std::unique_ptr<ISpritePolicy> policy;
  Transform2 transform{};
};

/**
 * Retained-mode scene: owns drawable entities and playfield description.
 * Render builds BlitterQueue + CopperProgram (not executed on host).
 */
class Scene {
public:
  explicit Scene(PlayfieldConfig pf);

  [[nodiscard]] EntityId add_drawable(LayerId layer, std::unique_ptr<ISpritePolicy> policy);

  void set_transform(EntityId id, Transform2 t);

  void update_fixed_step(float dt_seconds);

  void build_blitter_frame(BlitterQueue &out) const;

  [[nodiscard]] const PlayfieldConfig &playfield() const { return playfield_; }

private:
  PlayfieldConfig playfield_;
  std::vector<DrawableEntity> drawables_{};
  EntityId next_id_{1};
};

} // namespace amiga::engine
