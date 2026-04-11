#pragma once

#include <cstdint>

#include "lvgl.h"

// `bu_vu_strip` — segmented LED-style level display (vertical or horizontal).
//
// Display-only: `value` is the number of lit segments counting from the start
// edge (bottom for vertical, left for horizontal). Theme accent fills lit
// segments; outline tokens dim the rest.

namespace blusys::ui {

enum class vu_orientation : std::uint8_t {
    vertical,
    horizontal,
};

struct vu_strip_config {
    std::uint8_t   segment_count = 12;  // 1–24
    std::uint8_t   initial       = 0;
    vu_orientation orientation   = vu_orientation::vertical;
};

lv_obj_t *vu_strip_create(lv_obj_t *parent, const vu_strip_config &config);

void     vu_strip_set_value(lv_obj_t *strip, std::uint8_t value);
std::uint8_t vu_strip_get_value(lv_obj_t *strip);

}  // namespace blusys::ui
