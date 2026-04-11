#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/flows/settings.hpp"
#include "blusys/app/view/page.hpp"
#include "blusys/framework/ui/theme.hpp"
#include "blusys/framework/ui/widgets/button/button.hpp"
#include "blusys/framework/ui/widgets/toggle/toggle.hpp"
#include "blusys/framework/ui/widgets/slider/slider.hpp"
#include "blusys/framework/ui/widgets/dropdown/dropdown.hpp"
#include "blusys/framework/ui/primitives/key_value.hpp"
#include "blusys/framework/ui/primitives/label.hpp"
#include "blusys/framework/ui/primitives/divider.hpp"

namespace blusys::app::flows {

namespace {

struct item_cb_ctx {
    const settings_screen_config *cfg;
    std::size_t index;
};

// Small fixed pool for settings item callbacks.
static constexpr std::size_t kMaxSettingsCallbacks = 16;
item_cb_ctx g_item_ctx[kMaxSettingsCallbacks] = {};
std::size_t g_item_ctx_count = 0;

item_cb_ctx *acquire_item_ctx(const settings_screen_config *cfg, std::size_t idx)
{
    if (g_item_ctx_count >= kMaxSettingsCallbacks) {
        return nullptr;
    }
    auto *c = &g_item_ctx[g_item_ctx_count++];
    c->cfg = cfg;
    c->index = idx;
    return c;
}

void on_toggle_changed(bool new_state, void *user_data)
{
    auto *ic = static_cast<item_cb_ctx *>(user_data);
    if (ic != nullptr && ic->cfg != nullptr && ic->cfg->on_changed != nullptr) {
        const auto &item = ic->cfg->items[ic->index];
        const std::uint32_t sid =
            item.id != 0 ? item.id : static_cast<std::uint32_t>(ic->index);
        ic->cfg->on_changed(ic->cfg->user_ctx, sid, new_state ? 1 : 0);
    }
}

void on_slider_changed(std::int32_t new_value, void *user_data)
{
    auto *ic = static_cast<item_cb_ctx *>(user_data);
    if (ic != nullptr && ic->cfg != nullptr && ic->cfg->on_changed != nullptr) {
        const auto &item = ic->cfg->items[ic->index];
        const std::uint32_t sid =
            item.id != 0 ? item.id : static_cast<std::uint32_t>(ic->index);
        ic->cfg->on_changed(ic->cfg->user_ctx, sid, new_value);
    }
}

void on_dropdown_changed(std::int32_t index, void *user_data)
{
    auto *ic = static_cast<item_cb_ctx *>(user_data);
    if (ic != nullptr && ic->cfg != nullptr && ic->cfg->on_changed != nullptr) {
        const auto &item = ic->cfg->items[ic->index];
        const std::uint32_t sid =
            item.id != 0 ? item.id : static_cast<std::uint32_t>(ic->index);
        ic->cfg->on_changed(ic->cfg->user_ctx, sid, index);
    }
}

void on_button_pressed(void *user_data)
{
    auto *ic = static_cast<item_cb_ctx *>(user_data);
    if (ic != nullptr && ic->cfg != nullptr && ic->cfg->on_changed != nullptr) {
        const auto &item = ic->cfg->items[ic->index];
        const std::uint32_t sid =
            item.id != 0 ? item.id : static_cast<std::uint32_t>(ic->index);
        ic->cfg->on_changed(ic->cfg->user_ctx, sid, 0);
    }
}

}  // namespace

lv_obj_t *settings_screen_create(app_ctx & /*ctx*/, const void *params,
                                  lv_group_t **group_out)
{
    const auto *cfg = static_cast<const settings_screen_config *>(params);
    const auto &t   = blusys::ui::theme();

    // Reset callback pool — settings screens are not re-entrant.
    g_item_ctx_count = 0;

    auto page = view::page_create({.scrollable = true});

    // Title.
    if (cfg != nullptr && cfg->title != nullptr) {
        lv_obj_t *title = lv_label_create(page.content);
        lv_label_set_text(title, cfg->title);
        lv_obj_set_style_text_font(title, t.font_title, 0);
        lv_obj_set_style_text_color(title, t.color_on_surface, 0);
        lv_obj_set_width(title, LV_PCT(100));
    }

    if (cfg == nullptr || cfg->items == nullptr) {
        if (group_out != nullptr) { *group_out = page.group; }
        return page.screen;
    }

    for (std::size_t i = 0; i < cfg->item_count; ++i) {
        const auto &item = cfg->items[i];

        switch (item.type) {
        case setting_type::section_header: {
            blusys::ui::divider_create(page.content, {});
            if (item.label != nullptr) {
                lv_obj_t *hdr = lv_label_create(page.content);
                lv_label_set_text(hdr, item.label);
                lv_obj_set_style_text_font(hdr, t.font_label, 0);
                lv_obj_set_style_text_color(hdr, t.color_primary, 0);
                lv_obj_set_width(hdr, LV_PCT(100));
            }
            break;
        }
        case setting_type::info: {
            blusys::ui::key_value_create(page.content, {
                .key = item.label,
                .value = item.info_value,
            });
            break;
        }
        case setting_type::toggle: {
            // Row: label + toggle
            lv_obj_t *row = lv_obj_create(page.content);
            lv_obj_remove_style_all(row);
            lv_obj_set_size(row, LV_PCT(100), LV_SIZE_CONTENT);
            lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
            lv_obj_set_flex_align(row,
                                  LV_FLEX_ALIGN_SPACE_BETWEEN,
                                  LV_FLEX_ALIGN_CENTER,
                                  LV_FLEX_ALIGN_CENTER);

            if (item.label != nullptr) {
                lv_obj_t *lbl = lv_label_create(row);
                lv_label_set_text(lbl, item.label);
                lv_obj_set_style_text_font(lbl, t.font_body, 0);
                lv_obj_set_style_text_color(lbl, t.color_on_surface, 0);
                lv_obj_set_flex_grow(lbl, 1);
            }

            item_cb_ctx *ic = acquire_item_ctx(cfg, i);
            blusys::ui::toggle_config tcfg{};
            tcfg.initial   = item.toggle_initial;
            tcfg.on_change = on_toggle_changed;
            tcfg.user_data = ic;
            lv_obj_t *tgl = blusys::ui::toggle_create(row, tcfg);
            if (tgl != nullptr) {
                lv_group_add_obj(page.group, tgl);
            }
            break;
        }
        case setting_type::slider: {
            if (item.label != nullptr) {
                lv_obj_t *lbl = lv_label_create(page.content);
                lv_label_set_text(lbl, item.label);
                lv_obj_set_style_text_font(lbl, t.font_body, 0);
                lv_obj_set_style_text_color(lbl, t.color_on_surface, 0);
                lv_obj_set_width(lbl, LV_PCT(100));
            }

            item_cb_ctx *ic = acquire_item_ctx(cfg, i);
            blusys::ui::slider_config scfg{};
            scfg.min       = item.slider_min;
            scfg.max       = item.slider_max;
            scfg.initial   = item.slider_initial;
            scfg.on_change = on_slider_changed;
            scfg.user_data = ic;
            lv_obj_t *sl = blusys::ui::slider_create(page.content, scfg);
            if (sl != nullptr) {
                lv_group_add_obj(page.group, sl);
            }
            break;
        }
        case setting_type::dropdown: {
            if (item.label != nullptr) {
                lv_obj_t *lbl = lv_label_create(page.content);
                lv_label_set_text(lbl, item.label);
                lv_obj_set_style_text_font(lbl, t.font_body, 0);
                lv_obj_set_style_text_color(lbl, t.color_on_surface, 0);
                lv_obj_set_width(lbl, LV_PCT(100));
            }

            item_cb_ctx *ic = acquire_item_ctx(cfg, i);
            blusys::ui::dropdown_config dcfg{};
            dcfg.options   = item.dropdown_options;
            dcfg.option_count = item.dropdown_count;
            dcfg.initial   = item.dropdown_initial;
            dcfg.on_select = on_dropdown_changed;
            dcfg.user_data = ic;
            lv_obj_t *dd = blusys::ui::dropdown_create(page.content, dcfg);
            if (dd != nullptr) {
                lv_group_add_obj(page.group, dd);
            }
            break;
        }
        case setting_type::action_button: {
            item_cb_ctx *ic = acquire_item_ctx(cfg, i);
            blusys::ui::button_config bcfg{};
            bcfg.label     = item.button_label != nullptr ? item.button_label : item.label;
            bcfg.variant   = blusys::ui::button_variant::secondary;
            bcfg.on_press  = on_button_pressed;
            bcfg.user_data = ic;
            lv_obj_t *btn = blusys::ui::button_create(page.content, bcfg);
            if (btn != nullptr) {
                lv_group_add_obj(page.group, btn);
            }
            break;
        }
        }
    }

    if (group_out != nullptr) {
        *group_out = page.group;
    }
    return page.screen;
}

}  // namespace blusys::app::flows

#endif  // BLUSYS_FRAMEWORK_HAS_UI
