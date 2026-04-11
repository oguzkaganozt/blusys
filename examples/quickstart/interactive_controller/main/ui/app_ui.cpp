#include "ui/app_ui.hpp"

#include "blusys/app/flows/settings.hpp"
#include "blusys/app/screens/about_screen.hpp"
#include "blusys/app/view/action_widgets.hpp"
#include "blusys/app/view/overlay_manager.hpp"
#include "blusys/framework/ui/primitives/key_value.hpp"
#include "blusys/version.h"

namespace interactive_controller::system {
const char *controller_build_version_for_build();
const char *controller_profile_label_for_build();
const char *controller_hardware_label_for_build();
}

namespace interactive_controller::ui {

namespace view = blusys::app::view;

namespace {

app_state *g_state = nullptr;

view::page make_page(blusys::app::app_ctx &ctx, bool scrollable)
{
    if (ctx.shell() != nullptr) {
        return view::page_create_in(ctx.shell()->content_area, {.scrollable = scrollable});
    }
    return view::page_create({.scrollable = scrollable});
}

void clear_home_handles()
{
    if (g_state == nullptr) {
        return;
    }
    g_state->home_gauge = nullptr;
    g_state->home_preset = nullptr;
    g_state->home_hold_badge = nullptr;
}

void clear_status_handles()
{
    if (g_state == nullptr) {
        return;
    }
    g_state->status_setup = nullptr;
    g_state->status_storage = nullptr;
    g_state->status_output = nullptr;
    g_state->status_hold = nullptr;
}

void clear_setup_handles()
{
    if (g_state == nullptr) {
        return;
    }
    g_state->setup_handles = {};
}

void on_home_show(lv_obj_t * /*screen*/, void *user_data)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_data);
    if (ctx == nullptr || ctx->shell() == nullptr) {
        return;
    }
    view::shell_set_title(*ctx->shell(), "Controller");
    view::shell_set_active_tab(*ctx->shell(), 0);
}

void on_status_show(lv_obj_t * /*screen*/, void *user_data)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_data);
    if (ctx == nullptr || ctx->shell() == nullptr) {
        return;
    }
    view::shell_set_title(*ctx->shell(), "Status");
    view::shell_set_active_tab(*ctx->shell(), 1);
}

void on_settings_show(lv_obj_t * /*screen*/, void *user_data)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_data);
    if (ctx == nullptr || ctx->shell() == nullptr) {
        return;
    }
    view::shell_set_title(*ctx->shell(), "Settings");
    view::shell_set_active_tab(*ctx->shell(), 2);
}

void on_setup_show(lv_obj_t * /*screen*/, void *user_data)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_data);
    if (ctx != nullptr && ctx->shell() != nullptr) {
        view::shell_set_title(*ctx->shell(), "Setup");
    }
}

void on_about_show(lv_obj_t * /*screen*/, void *user_data)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_data);
    if (ctx != nullptr && ctx->shell() != nullptr) {
        view::shell_set_title(*ctx->shell(), "About");
    }
}

void on_settings_changed(void *user_ctx, std::size_t item_index, std::int32_t value)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_ctx);
    if (ctx == nullptr) {
        return;
    }

    switch (item_index) {
    case 1:
        ctx->dispatch(action{.tag = action_tag::set_accent, .value = value});
        break;
    case 2:
        ctx->dispatch(action{.tag = action_tag::toggle_hold});
        break;
    case 3:
        ctx->dispatch(action{.tag = action_tag::open_about});
        break;
    default:
        break;
    }
}

lv_obj_t *create_home(blusys::app::app_ctx &ctx, const void * /*params*/, lv_group_t **group_out)
{
    auto page = make_page(ctx, false);
    auto *hero = view::card(page.content, "Pulse Engine");
    lv_obj_set_width(hero, LV_PCT(100));

    view::label(hero, "Drive");
    g_state->home_gauge = view::gauge(hero, 0, 100, g_state->level, "%");
    g_state->home_preset = view::key_value(hero, "Accent", accent_name(g_state->accent_index));
    g_state->home_hold_badge = view::status_badge(hero,
                                                  g_state->hold_enabled ? "Hold" : "Live",
                                                  g_state->hold_enabled
                                                      ? blusys::ui::badge_level::warning
                                                      : blusys::ui::badge_level::success);

    auto *controls = view::row(page.content);
    view::button(controls, "Down", action{.tag = action_tag::level_delta, .value = -6}, &ctx,
                 blusys::ui::button_variant::secondary);
    view::button(controls, "Up", action{.tag = action_tag::level_delta, .value = 6}, &ctx,
                 blusys::ui::button_variant::secondary);
    view::button(controls, g_state->hold_enabled ? "Release" : "Hold",
                 action{.tag = action_tag::toggle_hold}, &ctx,
                 blusys::ui::button_variant::ghost);

    auto *actions = view::row(page.content);
    view::button(actions, "Setup", action{.tag = action_tag::open_setup}, &ctx,
                 blusys::ui::button_variant::secondary);
    view::button(actions, "Commit", action{.tag = action_tag::confirm}, &ctx);

    if (group_out != nullptr) {
        *group_out = page.group;
    }
    return ctx.shell() != nullptr ? page.content : page.screen;
}

lv_obj_t *create_status(blusys::app::app_ctx &ctx, const void * /*params*/, lv_group_t **group_out)
{
    auto page = make_page(ctx, true);

    auto *summary = view::card(page.content, "Snapshot");
    lv_obj_set_width(summary, LV_PCT(100));
    g_state->status_setup = view::status_badge(summary,
                                               g_state->provisioning.capability_ready ? "Provisioned" : "Waiting",
                                               g_state->provisioning.capability_ready
                                                   ? blusys::ui::badge_level::success
                                                   : blusys::ui::badge_level::warning);
    g_state->status_output = view::key_value(summary, "Output", "64%");
    g_state->status_hold = view::key_value(summary, "Hold", "Off");
    g_state->status_storage = view::key_value(summary, "Storage", "Pending");

    auto *actions = view::row(page.content);
    view::button(actions, "Run Setup", action{.tag = action_tag::open_setup}, &ctx,
                 blusys::ui::button_variant::secondary);
    view::button(actions, "Settings", action{.tag = action_tag::show_settings}, &ctx,
                 blusys::ui::button_variant::ghost);

    if (group_out != nullptr) {
        *group_out = page.group;
    }
    return ctx.shell() != nullptr ? page.content : page.screen;
}

lv_obj_t *create_settings(blusys::app::app_ctx &ctx, const void * /*params*/, lv_group_t **group_out)
{
    static constexpr const char *kAccentOptions[] = {"Warm", "Punch", "Night"};

    blusys::app::flows::setting_item items[] = {
        {
            .label = "Character",
            .type = blusys::app::flows::setting_type::section_header,
        },
        {
            .label = "Accent",
            .description = "Shift the controller voice.",
            .type = blusys::app::flows::setting_type::dropdown,
            .dropdown_options = kAccentOptions,
            .dropdown_count = 3,
            .dropdown_initial = g_state->accent_index,
        },
        {
            .label = "Hold Output",
            .description = "Freeze adjustments until released.",
            .type = blusys::app::flows::setting_type::toggle,
            .toggle_initial = g_state->hold_enabled,
        },
        {
            .label = "About Device",
            .description = "Inspect the reference build details.",
            .type = blusys::app::flows::setting_type::action_button,
            .button_label = "Open",
        },
    };

    const blusys::app::flows::settings_screen_config config{
        .title = "Controller Settings",
        .items = items,
        .item_count = sizeof(items) / sizeof(items[0]),
        .on_changed = on_settings_changed,
        .user_ctx = &ctx,
    };

    return blusys::app::flows::settings_screen_create(ctx, &config, group_out);
}

lv_obj_t *create_setup(blusys::app::app_ctx &ctx, const void * /*params*/, lv_group_t **group_out)
{
    const blusys::app::flows::provisioning_flow_config config{
        .title = "Pair Controller",
        .qr_label = "BLE bootstrap payload:",
        .waiting_msg = "Waiting for product credentials...",
        .success_msg = "Controller paired.",
        .error_msg = "Provisioning failed.",
    };

    return blusys::app::flows::provisioning_screen_create(
        ctx, config, g_state->setup_handles, group_out);
}

lv_obj_t *create_about(blusys::app::app_ctx &ctx, const void * /*params*/, lv_group_t **group_out)
{
    const blusys::app::screens::about_extra_field extras[] = {
        {.key = "Archetype", .value = "interactive controller"},
        {.key = "Profile", .value = interactive_controller::system::controller_profile_label_for_build()},
        {.key = "Identity", .value = "expressive_dark"},
    };

    const blusys::app::screens::about_screen_config config{
        .product_name = "Pulse Controller",
        .firmware_version = interactive_controller::system::controller_build_version_for_build(),
        .hardware_version = interactive_controller::system::controller_hardware_label_for_build(),
        .serial_number = "CTRL-0001",
        .build_date = __DATE__,
        .extras = extras,
        .extra_count = sizeof(extras) / sizeof(extras[0]),
    };

    return blusys::app::screens::about_screen_create(ctx, &config, group_out);
}

}  // namespace

void on_init(blusys::app::app_ctx &ctx, app_state &state)
{
    g_state = &state;

    if (ctx.shell() != nullptr) {
        const view::shell_tab_item tabs[] = {
            {.label = "Ctrl", .route_id = route_home},
            {.label = "Stat", .route_id = route_status},
            {.label = "Set", .route_id = route_settings},
        };
        view::shell_set_tabs(*ctx.shell(), tabs, sizeof(tabs) / sizeof(tabs[0]), &ctx);

        if (lv_obj_t *surface = view::shell_status_surface(*ctx.shell()); surface != nullptr) {
            state.shell_badge = view::status_badge(surface, "Setup", blusys::ui::badge_level::warning);
            state.shell_detail = view::label(surface, "Punch  64%", blusys::ui::theme().font_body_sm);
        }

        if (ctx.overlay_manager() != nullptr) {
            view::overlay_create(ctx.shell()->root, overlay_confirm,
                                 {.text = "Scene committed", .duration_ms = 1200},
                                 *ctx.overlay_manager());
        }
    }

    auto *router = ctx.screen_router();
    if (router == nullptr) {
        return;
    }

    router->register_screen(route_home, &create_home, nullptr,
                            {.on_show = on_home_show, .on_hide = +[](lv_obj_t *, void *) { clear_home_handles(); }, .user_data = &ctx});
    router->register_screen(route_status, &create_status, nullptr,
                            {.on_show = on_status_show, .on_hide = +[](lv_obj_t *, void *) { clear_status_handles(); }, .user_data = &ctx});
    router->register_screen(route_settings, &create_settings, nullptr,
                            {.on_show = on_settings_show, .user_data = &ctx});
    router->register_screen(route_setup, &create_setup, nullptr,
                            {.on_show = on_setup_show, .on_hide = +[](lv_obj_t *, void *) { clear_setup_handles(); }, .user_data = &ctx});
    router->register_screen(route_about, &create_about, nullptr,
                            {.on_show = on_about_show, .user_data = &ctx});

    ctx.navigate_to(route_home);
}

}  // namespace interactive_controller::ui
