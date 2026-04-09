#pragma once

#include <cstdint>

namespace amiga::engine {

/// Module boundary for multi-affect scroll / tile engines (see effects/tiles8, layers).
struct TileLayerConfig {
  std::uint16_t map_w{0}, map_h{0};
  std::uint16_t tile_w{16}, tile_h{16};
  std::int16_t scroll_x{0}, scroll_y{0};
};

class TileScrollModule {
public:
  virtual ~TileScrollModule() = default;
  virtual void set_layer(int index, TileLayerConfig cfg) = 0;
};

} // namespace amiga::engine
