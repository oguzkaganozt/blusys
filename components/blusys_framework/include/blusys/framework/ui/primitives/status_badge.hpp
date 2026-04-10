#pragma once

#include <cstdint>

#include "lvgl.h"

// `bu_status_badge` — a compact, colored status indicator pill.
//
// Visual intent: a small rounded pill displaying a short text label on
// a colored background derived from the semantic status colors (info,
// success, warning, error).
//
// Display-only primitive — no user interaction callbacks.

namespace blusys::ui {

enum class badge_level : uint8_t {
    info,
    success,
    warning,
    error,
};

struct status_badge_config {
    const char  *text  = nullptr;
    badge_level  level = badge_level::info;
};

lv_obj_t *status_badge_create(lv_obj_t *parent, const status_badge_config &config);

void status_badge_set_text(lv_obj_t *badge, const char *text);
void status_badge_set_level(lv_obj_t *badge, badge_level level);

}  // namespace blusys::ui
