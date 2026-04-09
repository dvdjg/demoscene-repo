#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <new>
#include <span>
#include <vector>

namespace amiga::engine {

/// Logical memory domain for resources (mirrors MEMF_CHIP / MEMF_FAST semantics).
enum class MemoryDomain : std::uint8_t { Chip = 1, Fast = 2 };

/**
 * Bump allocator for per-frame or per-scratch data (host: std::vector backend;
 * on Amiga you would back this with MemAlloc(MEMF_CHIP) arenas).
 */
class BumpArena {
public:
  explicit BumpArena(std::size_t cap) : storage_(cap) {}

  void reset() { top_ = 0; }

  [[nodiscard]] std::byte *allocate(std::size_t n, std::size_t align) {
    std::uintptr_t p =
        reinterpret_cast<std::uintptr_t>(storage_.data()) + top_;
    std::uintptr_t aligned = (p + align - 1) & ~(align - 1);
    std::size_t offset = aligned - reinterpret_cast<std::uintptr_t>(storage_.data());
    if (offset + n > storage_.size())
      return nullptr;
    top_ = offset + n;
    return reinterpret_cast<std::byte *>(aligned);
  }

  template <typename T, typename... Args>
  T *create(Args &&...args) {
    void *p = allocate(sizeof(T), alignof(T));
    if (!p)
      return nullptr;
    return new (p) T(std::forward<Args>(args)...);
  }

private:
  std::vector<std::byte> storage_;
  std::size_t top_{0};
};

template <typename T>
using Box = std::unique_ptr<T>;

} // namespace amiga::engine
