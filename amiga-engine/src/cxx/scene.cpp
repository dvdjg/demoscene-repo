#include <amiga_engine/scene.hpp>

#include <algorithm>

namespace amiga::engine {

static void stub_enqueue(BlitterQueue &q, Transform2 t) {
  (void)t;
  BlitOp op{};
  op.kind = BlitOpKind::FillRect;
  op.dst = {0, 0, 1, 1};
  op.fill_value = 1;
  q.enqueue(op);
}

void HardwareSpritePolicy::enqueue_draw(BlitterQueue &q, Transform2 t) const {
  (void)channel_;
  stub_enqueue(q, t);
}

void BobSpritePolicy::enqueue_draw(BlitterQueue &q, Transform2 t) const { stub_enqueue(q, t); }

void SoftSpritePolicy::enqueue_draw(BlitterQueue &q, Transform2 t) const { stub_enqueue(q, t); }

Scene::Scene(PlayfieldConfig pf) : playfield_(pf) {}

EntityId Scene::add_drawable(LayerId layer, std::unique_ptr<ISpritePolicy> policy) {
  DrawableEntity e{};
  e.id = next_id_++;
  e.layer = layer;
  e.policy = std::move(policy);
  drawables_.push_back(std::move(e));
  return drawables_.back().id;
}

void Scene::set_transform(EntityId id, Transform2 t) {
  for (auto &d : drawables_) {
    if (d.id == id) {
      d.transform = t;
      return;
    }
  }
}

void Scene::update_fixed_step(float dt_seconds) {
  (void)dt_seconds;
  /* Hook: animation graphs, physics, AI. */
}

void Scene::build_blitter_frame(BlitterQueue &out) const {
  out.clear();
  std::vector<const DrawableEntity *> order;
  order.reserve(drawables_.size());
  for (const auto &d : drawables_)
    order.push_back(&d);
  std::sort(order.begin(), order.end(), [](const DrawableEntity *a, const DrawableEntity *b) {
    return static_cast<int>(a->layer) < static_cast<int>(b->layer);
  });
  for (const DrawableEntity *d : order) {
    if (d->policy)
      d->policy->enqueue_draw(out, d->transform);
  }
}

} // namespace amiga::engine
