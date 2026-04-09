#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace amiga::engine {

struct AssetRef {
  std::string id;
  std::uint32_t disk_group{0};
};

struct ProjectManifest {
  std::string title;
  std::vector<AssetRef> bitmaps;
  std::vector<AssetRef> sounds;
};

} // namespace amiga::engine
