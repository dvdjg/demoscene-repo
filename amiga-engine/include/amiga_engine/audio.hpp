#pragma once

#include <string_view>

namespace amiga::engine {

struct AudioBus {
  virtual ~AudioBus() = default;
  virtual void play_music(std::string_view module_id) = 0;
  virtual void stop_music() = 0;
};

struct NullAudioBus final : AudioBus {
  void play_music(std::string_view) override {}
  void stop_music() override {}
};

} // namespace amiga::engine
