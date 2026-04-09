#include <amiga_engine/blitter_queue.hpp>
#include <amiga_engine/blitter_sim.hpp>
#include <amiga_engine/copper_ops.hpp>
#include <amiga_engine/game_loop.hpp>
#include <amiga_engine/hal_custom_regs.hpp>
#include <amiga_engine/scene.hpp>
#include <amiga_engine/sprite_policy.hpp>

#include <cstdlib>
#include <iostream>
#include <span>
#include <vector>

namespace {

[[nodiscard]] bool read_pixel(std::span<const std::uint8_t> plane, std::uint16_t w, std::uint16_t x,
                              std::uint16_t y) {
  const std::size_t rb = (static_cast<std::size_t>(w) + 7) / 8;
  const std::size_t idx = y * rb + x / 8;
  const unsigned bit = 7 - (x % 8);
  return (plane[idx] >> bit) & 1;
}


int g_pass = 0;
int g_fail = 0;
int g_test_index = 0;

void check(bool ok, const char *name) {
  ++g_test_index;
  if (ok) {
    ++g_pass;
    std::cout << "ok " << g_test_index << " - " << name << '\n';
  } else {
    ++g_fail;
    std::cout << "not ok " << g_test_index << " - " << name << '\n';
  }
}

} // namespace

int main() {
  std::cout << "TAP version 13\n";
  constexpr int k_planned = 8;
  std::cout << "1.." << k_planned << '\n';

  {
    amiga::engine::CopperProgramBuilder b;
    b.move(amiga::engine::hal::custom_reg_u16(amiga::engine::hal::CustomReg::color0),
           0x0f00);
    b.end();
    const auto w = b.encode_words();
    check(w.size() == 4, "copper encode move+end word count");
    check(w[0] == 0x0180 && w[1] == 0x0f00, "copper encode move color0");
    check(w[2] == 0xffff && w[3] == 0xfffe, "copper encode end");
  }

  {
    amiga::engine::CopperProgramBuilder b;
    b.wait(50, 10);
    const auto w = b.encode_words();
    check(w.size() == 2, "copper wait word count");
    const std::uint16_t hp_byte = static_cast<std::uint16_t>(((10 >> 1) | 1) & 0xff);
    const std::uint16_t expected = static_cast<std::uint16_t>((50u << 8) | hp_byte);
    check(w[0] == expected && w[1] == 0xfffe, "copper wait encoding");
  }

  {
    amiga::engine::BlitterQueue q;
    q.enqueue(amiga::engine::BlitOp{.kind = amiga::engine::BlitOpKind::FillRect,
                                   .dst = {2, 2, 4, 4},
                                   .fill_value = 1});
    amiga::engine::PlanarFramebuffer fb(16, 16);
    fb.clear(0);
    const std::vector<std::uint8_t> empty;
    amiga::engine::run_blitter_queue(q, fb, empty);
    check(read_pixel(fb.bytes(), 16, 3, 3), "blitter fill sets pixel");
  }

  {
    amiga::engine::PlayfieldConfig pf{};
    pf.depth = 3;
    amiga::engine::Scene scene(pf);
    amiga::engine::BlitterQueue q;
    const auto id =
        scene.add_drawable(amiga::engine::LayerId::Bobs,
                             std::make_unique<amiga::engine::BobSpritePolicy>());
    scene.set_transform(id, {10, 20});
    scene.build_blitter_frame(q);
    check(q.count() >= 1, "scene produces blit work");
  }

  {
    int ticks = 0;
    amiga::engine::GameLoop loop(1.f / 50.f, [&](float dt) {
      (void)dt;
      ++ticks;
    });
    loop.run_steps(5);
    check(ticks == 5, "game loop fixed steps");
  }

  if (g_test_index != k_planned || g_fail != 0)
    return EXIT_FAILURE;
  return EXIT_SUCCESS;
}
