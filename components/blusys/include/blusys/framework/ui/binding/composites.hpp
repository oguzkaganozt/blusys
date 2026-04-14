#pragma once

// Named, bounded view composites — call from update() only; same rules as bindings.hpp.

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/ui/primitives/status_badge.hpp"
#include "lvgl.h"

namespace blusys {

// Caption + value labels (e.g. settings row). Null pointers are skipped.
struct labeled_value_row {
    lv_obj_t *caption = nullptr;
    lv_obj_t *value   = nullptr;
};

void sync_labeled_value(const labeled_value_row &row, const char *value_text);
void sync_labeled_value(const labeled_value_row &row, const char *caption_text, const char *value_text);

// Status badge + optional supporting label line (e.g. connectivity / alarm strip).
struct status_strip {
    lv_obj_t *badge = nullptr;
    lv_obj_t *detail_label = nullptr;
};

void sync_status_strip(const status_strip &strip,
                       blusys::badge_level level,
                       const char *badge_text,
                       const char *detail_text);

// Up to four key-value widgets (e.g. operator dashboard row). Null slots or nullptr
// value strings are skipped.
struct key_value_quad {
    lv_obj_t *kv[4] = {};
};

void sync_key_value_quad(const key_value_quad &row,
                         const char *v0,
                         const char *v1,
                         const char *v2,
                         const char *v3);

// Three key-value widgets (e.g. load / temp / mode). Null slots skipped.
struct key_value_trio {
    lv_obj_t *kv[3] = {};
};

void sync_key_value_trio(const key_value_trio &row,
                         const char *v0,
                         const char *v1,
                         const char *v2);

}  // namespace blusys

#endif  // BLUSYS_FRAMEWORK_HAS_UI
