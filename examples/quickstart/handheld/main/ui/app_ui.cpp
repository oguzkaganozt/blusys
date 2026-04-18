#include "ui/app_ui.hpp"
#include "blusys/blusys.hpp"

#include <cstdint>

namespace handheld_starter::system {
const char *controller_build_version_for_build();
const char *controller_profile_label_for_build();
const char *controller_hardware_label_for_build();
}  // namespace handheld_starter::system

namespace handheld_starter::ui {

namespace view = blusys;

using handheld_starter::accent_name;
using handheld_starter::app_state;
using handheld_starter::action;
using handheld_starter::action_tag;
using handheld_starter::overlay;
using handheld_starter::route;

namespace {

static blusys::flows::provisioning_screen_handles s_setup_handles{};

enum settings_id : std::uint32_t {
    accent = 40,
    hold_toggle,
    about_action,
};

view::page make_page(blusys::app_ctx &ctx, bool scrollable)
{
    if (ctx.fx().nav.has_shell()) {
        return view::page_create_in(ctx.fx().nav.content_area(), {.scrollable = scrollable});
    }
    return view::page_create({.scrollable = scrollable});
}

void on_home_hide(lv_obj_t * /*screen*/, void *user_data)
{
    auto *ctx = static_cast<blusys::app_ctx *>(user_data);
    if (ctx == nullptr) {
        return;
    }
    auto *st = ctx->product_state<app_state>();
    if (st != nullptr) {
        st->home.clear();
    }
}

void on_status_hide(lv_obj_t * /*screen*/, void *user_data)
{
    auto *ctx = static_cast<blusys::app_ctx *>(user_data);
    if (ctx == nullptr) {
        return;
    }
    auto *st = ctx->product_state<app_state>();
    if (st != nullptr) {
        st->status.clear();
    }
}

void on_setup_hide(lv_obj_t * /*screen*/, void * /*user_data*/)
{
    s_setup_handles = {};
}

void on_setup_show(lv_obj_t * /*screen*/, void *user_data)
{
    auto *ctx = static_cast<blusys::app_ctx *>(user_data);
    if (ctx != nullptr) {
        ctx->fx().nav.set_title("Setup");
    }
}

void on_about_show(lv_obj_t * /*screen*/, void *user_data)
{
    auto *ctx = static_cast<blusys::app_ctx *>(user_data);
    if (ctx != nullptr) {
        ctx->fx().nav.set_title("About");
    }
}

void on_status_empty_setup(void *user_data)
{
    auto *ctx = static_cast<blusys::app_ctx *>(user_data);
    if (ctx != nullptr) {
        ctx->dispatch(action{.tag = action_tag::open_setup});
    }
}

void on_settings_changed(void *user_ctx, std::uint32_t setting_id, std::int32_t value)
{
    auto *ctx = static_cast<blusys::app_ctx *>(user_ctx);
    if (ctx == nullptr) {
        return;
    }

    switch (setting_id) {
    case accent:
        ctx->dispatch(action{.tag = action_tag::set_accent, .value = value});
        break;
    case hold_toggle:
        ctx->dispatch(action{.tag = action_tag::toggle_hold});
        break;
    case about_action:
        ctx->dispatch(action{.tag = action_tag::open_about});
        break;
    default:
        break;
    }
}

lv_obj_t *create_home(blusys::app_ctx &ctx, const void * /*params*/, lv_group_t **group_out)
{
    auto *st = ctx.product_state<app_state>();
    if (st == nullptr) {
        return nullptr;
    }

    auto page = make_page(ctx, false);
    auto *hero = view::card(page.content, "Pulse Engine");
    lv_obj_set_width(hero, LV_PCT(100));

    view::label(hero, "Drive");
    st->home.output.gauge = view::gauge(hero, 0, 100, st->level, "%");

    auto *meters = view::row(page.content);
    lv_obj_set_width(meters, LV_PCT(100));
    const auto vu_initial = static_cast<std::uint8_t>((st->level * 12) / 100);
    st->home.output.vu_strip =
        view::vu_strip(meters, 12, vu_initial, blusys::vu_orientation::vertical);
    st->home.output.level_bar =
        view::level_bar(meters, 0, 100, st->level, "Output");
    st->home.output.vu_segment_count = 12;
    lv_obj_set_flex_grow(st->home.output.level_bar, 1);
    st->home.preset_kv = view::key_value(hero, "Accent", accent_name(st->accent_index));
    st->home.hold_badge = view::status_badge(hero,
                                                  st->hold_enabled ? "Hold" : "Live",
                                                  st->hold_enabled
                                                      ? blusys::badge_level::warning
                                                      : blusys::badge_level::success);

    auto *controls = view::row(page.content);
    view::button(controls, "Down", action{.tag = action_tag::level_delta, .value = -6}, &ctx,
                 blusys::button_variant::secondary);
    view::button(controls, "Up", action{.tag = action_tag::level_delta, .value = 6}, &ctx,
                 blusys::button_variant::secondary);
    view::button(controls, st->hold_enabled ? "Release" : "Hold",
                 action{.tag = action_tag::toggle_hold}, &ctx,
                 blusys::button_variant::ghost);

    auto *actions = view::row(page.content);
    view::button(actions, "Setup", action{.tag = action_tag::open_setup}, &ctx,
                 blusys::button_variant::secondary);
    view::button(actions, "Commit", action{.tag = action_tag::confirm}, &ctx);

    if (group_out != nullptr) {
        *group_out = page.group;
    }
    return ctx.fx().nav.has_shell() ? page.content : page.screen;
}

lv_obj_t *create_status(blusys::app_ctx &ctx, const void * /*params*/, lv_group_t **group_out)
{
    auto *st = ctx.product_state<app_state>();
    if (st == nullptr) {
        return nullptr;
    }

    auto page = make_page(ctx, true);

    const bool setup_ready =
        st->provisioning.capability_ready || st->provisioning.is_provisioned;
    if (!setup_ready) {
        lv_obj_t *empty = blusys::flows::empty_state_create(
            page.content,
            {
                .title = "Provisioning pending",
                .message = "Finish setup to enable live status and storage.",
                .primary_label = "Run setup",
                .on_primary = on_status_empty_setup,
                .primary_user_data = &ctx,
            });
        lv_obj_set_width(empty, LV_PCT(100));
    }

    auto *summary = view::card(page.content, "Snapshot");
    lv_obj_set_width(summary, LV_PCT(100));
    st->status.setup_badge = view::status_badge(summary,
                                                       setup_ready ? "Provisioned" : "Waiting",
                                                       setup_ready ? blusys::badge_level::success
                                                                   : blusys::badge_level::warning);
    st->status.output_kv = view::key_value(summary, "Output", "64%");
    st->status.hold_kv = view::key_value(summary, "Hold", "Off");
    st->status.storage_kv = view::key_value(summary, "Storage", "Pending");

    auto *actions = view::row(page.content);
    view::button(actions, "Run Setup", action{.tag = action_tag::open_setup}, &ctx,
                 blusys::button_variant::secondary);
    view::button(actions, "Settings", action{.tag = action_tag::show_settings}, &ctx,
                 blusys::button_variant::ghost);

    if (group_out != nullptr) {
        *group_out = page.group;
    }
    return ctx.fx().nav.has_shell() ? page.content : page.screen;
}

lv_obj_t *create_settings(blusys::app_ctx &ctx, const void * /*params*/, lv_group_t **group_out)
{
    auto *st = ctx.product_state<app_state>();
    if (st == nullptr) {
        return nullptr;
    }

    static constexpr const char *kAccentOptions[] = {"Warm", "Punch", "Night"};

    blusys::flows::setting_item items[] = {
        {
            .label = "Character",
            .type = blusys::flows::setting_type::section_header,
        },
        {
            .label = "Accent",
            .description = "Shift the controller voice.",
            .type = blusys::flows::setting_type::dropdown,
            .id = accent,
            .dropdown_options = kAccentOptions,
            .dropdown_count = 3,
            .dropdown_initial = st->accent_index,
        },
        {
            .label = "Hold Output",
            .description = "Freeze adjustments until released.",
            .type = blusys::flows::setting_type::toggle,
            .id = hold_toggle,
            .toggle_initial = st->hold_enabled,
        },
        {
            .label = "About Device",
            .description = "Inspect the reference build details.",
            .type = blusys::flows::setting_type::action_button,
            .id = about_action,
            .button_label = "Open",
        },
    };

    const blusys::flows::settings_screen_config config{
        .title = "Controller Settings",
        .items = items,
        .item_count = sizeof(items) / sizeof(items[0]),
        .on_changed = on_settings_changed,
        .user_ctx = &ctx,
    };

    return blusys::flows::settings_screen_create(ctx, &config, group_out);
}

lv_obj_t *create_setup(blusys::app_ctx &ctx, const void * /*params*/, lv_group_t **group_out)
{
    auto *st = ctx.product_state<app_state>();
    if (st == nullptr) {
        return nullptr;
    }

    const blusys::flows::provisioning_flow_config config{
        .title = "Pair Controller",
        .qr_label = "BLE bootstrap payload:",
        .waiting_msg = "Waiting for product credentials...",
        .success_msg = "Controller paired.",
        .error_msg = "Provisioning failed.",
    };

    return blusys::flows::provisioning_screen_create(
        ctx, config, s_setup_handles, group_out);
}

lv_obj_t *create_about(blusys::app_ctx &ctx, const void * /*params*/, lv_group_t **group_out)
{
    const auto *build = ctx.fx().build.status();
    const blusys::screens::about_extra_field extras[] = {
        {.key = "Interface", .value = "handheld"},
        {.key = "Profile", .value = handheld_starter::system::controller_profile_label_for_build()},
        {.key = "Identity", .value = "expressive_dark"},
        {.key = "Build host", .value = build != nullptr ? build->build_host : "unknown"},
        {.key = "Commit", .value = build != nullptr ? build->commit_hash : "unknown"},
        {.key = "Features", .value = build != nullptr ? build->feature_flags : "unknown"},
    };

    const blusys::screens::about_screen_config config{
        .product_name = "Pulse Controller",
        .firmware_version = build != nullptr ? build->firmware_version
                                             : handheld_starter::system::controller_build_version_for_build(),
        .hardware_version = handheld_starter::system::controller_hardware_label_for_build(),
        .serial_number = "CTRL-0001",
        .build_date = __DATE__,
        .extras = extras,
        .extra_count = sizeof(extras) / sizeof(extras[0]),
    };

    return blusys::screens::about_screen_create(ctx, &config, group_out);
}

void register_all_screens(blusys::app_ctx &ctx)
{
    static const view::screen_registration kRoutes[] = {
        view::screen_row(route::home, &create_home, nullptr, nullptr, &on_home_hide),
        view::screen_row(route::status, &create_status, nullptr, nullptr, &on_status_hide),
        view::screen_row(route::about, &create_about, nullptr, &on_about_show, nullptr),
    };

    ctx.fx().nav.register_screens(&ctx, kRoutes, sizeof(kRoutes) / sizeof(kRoutes[0]));
}

}  // namespace

blusys::flows::settings_flow kSettingsFlow = []() {
    blusys::flows::settings_flow f;
    f.route_id  = static_cast<std::uint32_t>(route::settings);
    f.create_fn = create_settings;
    return f;
}();

blusys::flows::provisioning_flow kProvisioningFlow = []() {
    blusys::flows::provisioning_flow f;
    f.route_id         = static_cast<std::uint32_t>(route::setup);
    f.create_fn        = create_setup;
    f.lifecycle.on_show = on_setup_show;
    f.lifecycle.on_hide = on_setup_hide;
    return f;
}();

void update_provisioning_ui(const blusys::connectivity_provisioning_status &status)
{
    blusys::flows::provisioning_screen_update(s_setup_handles, status);
}

void on_init(blusys::app_ctx &ctx, blusys::app_fx &fx, app_state &state)
{
    (void)fx;
    (void)state;

    if (ctx.fx().nav.has_shell()) {
        const view::shell_tab_item tabs[] = {
            view::shell_tab_row("Ctrl", route::home, "Controller"),
            view::shell_tab_row("Stat", route::status, "Status"),
            view::shell_tab_row("Set", route::settings, "Settings"),
        };
        ctx.fx().nav.set_tab_items(tabs, sizeof(tabs) / sizeof(tabs[0]));

        if (lv_obj_t *surface = ctx.fx().nav.status_surface(); surface != nullptr) {
            state.shell.badge = view::status_badge(surface, "Setup", blusys::badge_level::warning);
            state.shell.detail =
                view::label(surface, "Punch  64%", blusys::theme().font_body_sm);
        }

        ctx.fx().nav.create_overlay(ctx.fx().nav.shell_root(),
                                    overlay::confirm,
                                    {.text = "Scene committed", .duration_ms = 1200});
    }

    register_all_screens(ctx);

    ctx.fx().nav.to(route::home);
}

}  // namespace handheld_starter::ui
