#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "lvgl.h"

namespace blusys {

// Explicit raw LVGL scope marker.
//
// Use this inside on_init or a custom widget to mark a block of raw
// LVGL calls as intentional. This is a no-op at runtime — it exists
// to make raw LVGL usage visible in code review and grep-able:
//
//   grep -r "lvgl_scope" src/
//
// Usage:
//
//   blusys::lvgl_scope(parent, [](lv_obj_t *p) {
//       lv_obj_t *arc = lv_arc_create(p);
//       lv_arc_set_range(arc, 0, 360);
//       lv_arc_set_value(arc, 180);
//   });
//
// Raw LVGL inside this scope must not:
//   - manage app screens directly
//   - manage UI locks directly
//   - call services directly
//   - manipulate routing or runtime internals

template <typename Fn>
void lvgl_scope(lv_obj_t *parent, Fn &&fn)
{
    fn(parent);
}

}  // namespace blusys

#endif  // BLUSYS_FRAMEWORK_HAS_UI
