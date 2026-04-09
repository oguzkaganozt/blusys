// framework_ui_basic — canonical example for the framework UI surface.
//
// This example covers two of the four framework example scenarios:
//
//   1. Layout primitives only — the screen + col + row + label + divider
//      composition at the top of app_main demonstrates the layout helpers
//      in isolation, before any interactive widget is created.
//   2. Interactive widgets — button (primary + secondary), toggle, slider,
//      modal, and overlay are wired up further down to demonstrate the
//      full V1 widget kit and the semantic callbacks contract.
//
// The remaining two framework example scenarios live in their own examples:
//
//   - framework_core_basic: spine in isolation (router, intent, controller,
//     runtime), no LVGL.
//   - framework_app_basic: full spine + widgets, button-driven event flow
//     through runtime.post_intent → controller → ui_route_sink.
//   - framework_encoder_basic: real lv_indev_t encoder driving
//     create_encoder_group + auto_focus_screen end-to-end.
//
// If you're looking for a layout-only or widget-kit-only starting point,
// this is the canonical example — do not duplicate it.

#include "blusys/framework/ui/widgets.hpp"

#include "blusys/log.h"

namespace {

constexpr const char *kTag = "framework_ui_basic";

int     g_press_count    = 0;
bool    g_toggle_state   = false;
int32_t g_slider_value   = 25;
int     g_modal_dismisses = 0;
int     g_overlay_hidden  = 0;

}  // namespace

extern "C" void app_main(void)
{
    lv_init();

    blusys::ui::set_theme({
        .color_primary = lv_color_hex(0x2A62FF),
        .color_surface = lv_color_hex(0x11141D),
        .color_on_primary = lv_color_hex(0xF7F9FF),
        .color_accent = lv_color_hex(0x69D6C8),
        .color_disabled = lv_color_hex(0x667089),
        .spacing_sm = 8,
        .spacing_md = 12,
        .spacing_lg = 20,
        .radius_card = 14,
        .radius_button = 999,
        .font_body = LV_FONT_DEFAULT,
        .font_title = LV_FONT_DEFAULT,
        .font_mono = LV_FONT_DEFAULT,
    });

    lv_obj_t *screen = blusys::ui::screen_create({});
    lv_obj_t *col = blusys::ui::col_create(screen, {
        .gap = blusys::ui::theme().spacing_md,
        .padding = blusys::ui::theme().spacing_lg,
    });
    blusys::ui::label_create(col, {
        .text = "Framework UI",
        .font = blusys::ui::theme().font_title,
    });
    blusys::ui::divider_create(col, {});
    lv_obj_t *row = blusys::ui::row_create(col, {
        .gap = blusys::ui::theme().spacing_sm,
        .padding = 0,
    });
    blusys::ui::label_create(row, {
        .text = "Theme and primitives compile.",
        .font = blusys::ui::theme().font_body,
    });

    lv_obj_t *button_row = blusys::ui::row_create(col, {
        .gap = blusys::ui::theme().spacing_sm,
        .padding = 0,
    });
    lv_obj_t *primary = blusys::ui::button_create(button_row, {
        .label = "Record",
        .variant = blusys::ui::button_variant::primary,
        .on_press = +[](void *user_data) {
            auto *count = static_cast<int *>(user_data);
            ++(*count);
            BLUSYS_LOGI(kTag, "primary pressed (count=%d)", *count);
        },
        .user_data = &g_press_count,
    });
    lv_obj_t *secondary = blusys::ui::button_create(button_row, {
        .label = "Disabled",
        .variant = blusys::ui::button_variant::secondary,
        .disabled = true,
    });

    lv_obj_t *toggle_row = blusys::ui::row_create(col, {
        .gap = blusys::ui::theme().spacing_md,
        .padding = 0,
    });
    blusys::ui::label_create(toggle_row, {
        .text = "Mute",
        .font = blusys::ui::theme().font_body,
    });
    lv_obj_t *toggle = blusys::ui::toggle_create(toggle_row, {
        .initial = false,
        .on_change = +[](bool new_state, void *user_data) {
            auto *flag = static_cast<bool *>(user_data);
            *flag = new_state;
            BLUSYS_LOGI(kTag, "toggle changed (state=%s)", new_state ? "on" : "off");
        },
        .user_data = &g_toggle_state,
    });

    lv_obj_t *slider_row = blusys::ui::row_create(col, {
        .gap = blusys::ui::theme().spacing_md,
        .padding = 0,
    });
    blusys::ui::label_create(slider_row, {
        .text = "Volume",
        .font = blusys::ui::theme().font_body,
    });
    lv_obj_t *slider = blusys::ui::slider_create(slider_row, {
        .min = 0,
        .max = 100,
        .initial = g_slider_value,
        .on_change = +[](int32_t new_value, void *user_data) {
            auto *value = static_cast<int32_t *>(user_data);
            *value = new_value;
            BLUSYS_LOGI(kTag, "slider changed (value=%ld)", static_cast<long>(new_value));
        },
        .user_data = &g_slider_value,
    });

    // Modals attach to the screen, not to a row inside the column.
    // Created hidden by convention; the example does not show it on
    // boot — this exercises the create + setter API at compile time
    // without obscuring the rest of the screen during visual testing.
    lv_obj_t *modal = blusys::ui::modal_create(screen, {
        .title = "Save changes?",
        .body  = "Tap outside the panel to dismiss this dialog.",
        .on_dismiss = +[](void *user_data) {
            auto *count = static_cast<int *>(user_data);
            ++(*count);
            BLUSYS_LOGI(kTag, "modal dismissed (count=%d)", *count);
        },
        .user_data = &g_modal_dismisses,
    });

    // Overlays attach to the screen and anchor themselves to bottom-mid.
    // Created hidden by convention; product would call overlay_show in
    // response to a save/error/event notification.
    lv_obj_t *overlay = blusys::ui::overlay_create(screen, {
        .text = "Saved",
        .duration_ms = 2500,
        .on_hidden = +[](void *user_data) {
            auto *count = static_cast<int *>(user_data);
            ++(*count);
            BLUSYS_LOGI(kTag, "overlay hidden (count=%d)", *count);
        },
        .user_data = &g_overlay_hidden,
    });

    // Wire encoder focus traversal across the screen. This example only
    // exercises the helper API at compile + runtime time — it does not
    // attach a real lv_indev_t. For an end-to-end encoder example with a
    // real LV_INDEV_TYPE_ENCODER device (SDL2 keyboard on host, encoder
    // driver on hardware), see examples/framework_encoder_basic/.
    lv_group_t *encoder_group = blusys::ui::create_encoder_group();
    blusys::ui::auto_focus_screen(screen, encoder_group);

    BLUSYS_LOGI(kTag,
                "screen=%p row=%p primary=%p secondary=%p toggle=%p slider=%p modal=%p overlay=%p group=%p",
                static_cast<void *>(screen),
                static_cast<void *>(row),
                static_cast<void *>(primary),
                static_cast<void *>(secondary),
                static_cast<void *>(toggle),
                static_cast<void *>(slider),
                static_cast<void *>(modal),
                static_cast<void *>(overlay),
                static_cast<void *>(encoder_group));
}
