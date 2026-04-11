#include "blusys/framework/ui/widgets/vu_strip/vu_strip.hpp"

#include "blusys/framework/ui/theme.hpp"
#include "blusys/log.h"

namespace blusys::ui {
namespace {

constexpr const char *kTag = "ui.vu_strip";

struct vu_strip_meta {
    std::uint8_t   count;
    std::uint8_t   value;
    vu_orientation orient;
    bool           in_use;
};

#ifndef BLUSYS_UI_VU_STRIP_POOL_SIZE
#define BLUSYS_UI_VU_STRIP_POOL_SIZE 16
#endif

vu_strip_meta g_vu_meta[BLUSYS_UI_VU_STRIP_POOL_SIZE];

vu_strip_meta *acquire_meta()
{
    for (auto &m : g_vu_meta) {
        if (!m.in_use) {
            m.in_use = true;
            return &m;
        }
    }
    BLUSYS_LOGE(kTag,
                "vu_strip meta pool exhausted (size=%d) — raise BLUSYS_UI_VU_STRIP_POOL_SIZE",
                BLUSYS_UI_VU_STRIP_POOL_SIZE);
    return nullptr;
}

void release_meta(vu_strip_meta *m)
{
    if (m != nullptr) {
        m->in_use = false;
    }
}

void apply_segment_style(lv_obj_t *seg, bool lit)
{
    const auto &t = theme();
    lv_obj_set_style_bg_color(seg, lit ? t.color_accent : t.color_outline_variant, 0);
    lv_obj_set_style_bg_opa(seg, LV_OPA_COVER, 0);
    const int r = t.radius_button > 2 ? t.radius_button / 2 : 1;
    lv_obj_set_style_radius(seg, r, 0);
    lv_obj_set_style_border_width(seg, 0, 0);
    lv_obj_remove_flag(seg, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(seg, LV_OBJ_FLAG_CLICKABLE);
}

void refresh_strip(lv_obj_t *strip, vu_strip_meta *m)
{
    const uint32_t n = lv_obj_get_child_count(strip);
    for (uint32_t i = 0; i < n; ++i) {
        lv_obj_t *seg = lv_obj_get_child(strip, i);
        const bool  lit = static_cast<std::uint8_t>(i) < m->value;
        apply_segment_style(seg, lit);
    }
}

void on_strip_delete(lv_event_t *e)
{
    auto *strip = static_cast<lv_obj_t *>(lv_event_get_target(e));
    auto *meta  = static_cast<vu_strip_meta *>(lv_obj_get_user_data(strip));
    release_meta(meta);
}

}  // namespace

lv_obj_t *vu_strip_create(lv_obj_t *parent, const vu_strip_config &config)
{
    vu_strip_meta *meta = acquire_meta();
    if (meta == nullptr) {
        return nullptr;
    }

    std::uint8_t n = config.segment_count;
    if (n == 0) {
        n = 12;
    }
    if (n > 24) {
        n = 24;
    }

    meta->orient = config.orientation;
    meta->count  = n;
    meta->value  = config.initial > n ? n : config.initial;

    const auto &t = theme();

    lv_obj_t *cont = lv_obj_create(parent);
    lv_obj_set_layout(cont, LV_LAYOUT_FLEX);
    lv_obj_set_style_bg_opa(cont, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_width(cont, 0, 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_user_data(cont, meta);
    lv_obj_add_event_cb(cont, on_strip_delete, LV_EVENT_DELETE, nullptr);

    const int gap = t.spacing_xs > 1 ? t.spacing_xs / 2 : 1;

    if (meta->orient == vu_orientation::vertical) {
        lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_COLUMN_REVERSE);
        lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
        lv_obj_set_style_pad_row(cont, gap, 0);
        lv_obj_set_width(cont, t.spacing_md);
        lv_obj_set_height(cont, LV_SIZE_CONTENT);

        for (std::uint8_t i = 0; i < n; ++i) {
            lv_obj_t *seg = lv_obj_create(cont);
            lv_obj_set_width(seg, LV_PCT(100));
            lv_obj_set_height(seg, t.spacing_sm);
            apply_segment_style(seg, static_cast<std::uint8_t>(i) < meta->value);
        }
    } else {
        lv_obj_set_flex_flow(cont, LV_FLEX_FLOW_ROW);
        lv_obj_set_flex_align(cont, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
        lv_obj_set_style_pad_column(cont, gap, 0);
        lv_obj_set_width(cont, LV_PCT(100));
        lv_obj_set_height(cont, t.spacing_md);

        for (std::uint8_t i = 0; i < n; ++i) {
            lv_obj_t *seg = lv_obj_create(cont);
            lv_obj_set_flex_grow(seg, 1);
            lv_obj_set_height(seg, t.spacing_sm);
            apply_segment_style(seg, static_cast<std::uint8_t>(i) < meta->value);
        }
    }

    return cont;
}

void vu_strip_set_value(lv_obj_t *strip, std::uint8_t value)
{
    if (strip == nullptr) {
        return;
    }
    auto *meta = static_cast<vu_strip_meta *>(lv_obj_get_user_data(strip));
    if (meta == nullptr) {
        return;
    }
    if (value > meta->count) {
        value = meta->count;
    }
    meta->value = value;
    refresh_strip(strip, meta);
}

std::uint8_t vu_strip_get_value(lv_obj_t *strip)
{
    if (strip == nullptr) {
        return 0;
    }
    auto *meta = static_cast<vu_strip_meta *>(lv_obj_get_user_data(strip));
    return meta != nullptr ? meta->value : 0;
}

}  // namespace blusys::ui
