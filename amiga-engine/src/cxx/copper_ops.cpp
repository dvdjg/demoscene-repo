#include <amiga_engine/copper_ops.hpp>

#include <type_traits>

namespace amiga::engine {

std::vector<std::uint16_t> CopperProgramBuilder::encode_words() const {
  std::vector<std::uint16_t> out;
  for (const auto &ins : ops_) {
    std::visit(
        [&out](auto &&arg) {
          using T = std::decay_t<decltype(arg)>;
          if constexpr (std::is_same_v<T, CopperMove>) {
            out.push_back(arg.reg_offset);
            out.push_back(arg.value);
          } else if constexpr (std::is_same_v<T, CopperWait>) {
            const std::uint16_t hp = static_cast<std::uint16_t>((arg.hp >> 1) | 1);
            out.push_back(static_cast<std::uint16_t>((static_cast<std::uint16_t>(arg.vp) << 8) | (hp & 0xff)));
            out.push_back(0xfffe);
          } else if constexpr (std::is_same_v<T, CopperEnd>) {
            /* Same terminal as CopListFinish: stli(..., 0xfffffffe). */
            out.push_back(0xffff);
            out.push_back(0xfffe);
          }
        },
        ins);
  }
  return out;
}

} // namespace amiga::engine
