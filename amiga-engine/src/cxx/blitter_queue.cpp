#include <amiga_engine/blitter_queue.hpp>

namespace amiga::engine {

bool BlitterQueue::set_budget(std::size_t max_ops) {
  if (ops_.size() <= max_ops)
    return true;
  ops_.resize(max_ops);
  return false;
}

} // namespace amiga::engine
