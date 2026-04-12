#include "ui/app_ui.hpp"

#include "blusys/app/screens/about_screen.hpp"
#include "blusys/app/view/action_widgets.hpp"

#include "lvgl.h"

namespace interactive_dashboard::system {
const char *dashboard_build_version_for_build();
const char *dashboard_profile_label_for_build();
const char *dashboard_hardware_label_for_build();
}  // namespace interactive_dashboard::system

namespace interactive_dashboard::ui {

namespace view = blusys::app::view;

using interactive_dashboard::action;
using interactive_dashboard::action_tag;
using interactive_dashboard::app_state;
using interactive_dashboard::route_overview;
using interactive_dashboard::route_about;

namespace {

bool dashboard_short_viewport()
{
    const lv_disp_t *d = lv_disp_get_default();
    if (d == nullptr) {
        return true;
    }
    return lv_disp_get_ver_res(d) <= 340;
}

bool dashboard_narrow_viewport()
{
    const lv_disp_t *d = lv_disp_get_default();
    if (d == nullptr) {
        return true;
    }
    return lv_disp_get_hor_res(d) <= 400;
}

view::page make_page(blusys::app::app_ctx &ctx, bool scrollable, bool compact)
{
    view::page_config cfg{.scrollable = scrollable};
    if (compact) {
        cfg.padding = 8;
        cfg.gap     = 6;
    }
    if (ctx.shell() != nullptr) {
        return view::page_create_in(ctx.shell()->content_area, cfg);
    }
    return view::page_create(cfg);
}

void on_overview_hide(lv_obj_t * /*screen*/, void *user_data)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_data);
    if (ctx == nullptr) {
        return;
    }
    auto *st = ctx->product_state<app_state>();
    if (st == nullptr) {
        return;
    }
    st->kpi_ops          = nullptr;
    st->kpi_temp         = nullptr;
    st->kpi_queue        = nullptr;
    st->dash_gauge       = nullptr;
    st->dash_line_chart  = nullptr;
    st->dash_bar_chart   = nullptr;
    st->line_series      = -1;
    st->bar_series       = -1;
}

void on_overview_show(lv_obj_t * /*screen*/, void *user_data)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_data);
    if (ctx == nullptr || ctx->shell() == nullptr) {
        return;
    }
    view::shell_set_title(*ctx->shell(), "Live ops");
    view::shell_set_active_tab(*ctx->shell(), 0);
}

void on_about_show(lv_obj_t * /*screen*/, void *user_data)
{
    auto *ctx = static_cast<blusys::app::app_ctx *>(user_data);
    if (ctx == nullptr || ctx->shell() == nullptr) {
        return;
    }
    view::shell_set_title(*ctx->shell(), "About");
    view::shell_set_active_tab(*ctx->shell(), 1);
}

action make_set_target(std::int32_t v)
{
    return action{.tag = action_tag::set_target, .value = v};
}

lv_obj_t *create_overview(blusys::app::app_ctx &ctx, const void * /*params*/, lv_group_t **group_out)
{
    auto *st = ctx.product_state<app_state>();
    if (st == nullptr) {
        return nullptr;
    }

    const bool compact  = dashboard_short_viewport();
    const bool narrow   = dashboard_narrow_viewport();
    auto       page     = make_page(ctx, true, compact);

    const int  split_h  = compact ? 76 : 124;
    const int  bar_h    = compact ? 56 : 96;
    const int  card_pad = compact ? 6 : -1;

    // Wide viewports: stacked KPI rows read best. Narrow (e.g. 320-wide): one row
    // of three metrics so the Snapshot card does not consume most of the viewport.
    auto *kpi_card = view::card(page.content, "Snapshot", card_pad);
    lv_obj_set_width(kpi_card, LV_PCT(100));
    if (compact) {
        lv_obj_set_style_pad_row(kpi_card, 2, 0);
    }

    if (narrow) {
        auto *kpi_row = view::row(kpi_card);
        lv_obj_set_width(kpi_row, LV_PCT(100));
        st->kpi_ops   = view::key_value(kpi_row, "Thr", "—");
        st->kpi_temp  = view::key_value(kpi_row, "In", "—");
        st->kpi_queue = view::key_value(kpi_row, "Q", "—");
        lv_obj_set_flex_grow(st->kpi_ops, 1);
        lv_obj_set_flex_grow(st->kpi_temp, 1);
        lv_obj_set_flex_grow(st->kpi_queue, 1);
    } else {
        st->kpi_ops   = view::key_value(kpi_card, "Throughput", "—");
        st->kpi_temp  = view::key_value(kpi_card, "Inlet", "—");
        st->kpi_queue = view::key_value(kpi_card, "Queue", "—");
    }

    auto *split_card = view::card(page.content, "Load & trend", card_pad);
    lv_obj_set_width(split_card, LV_PCT(100));
    auto *split = view::row(split_card);
    lv_obj_set_width(split, LV_PCT(100));
    lv_obj_set_height(split, split_h);

    st->dash_gauge = view::gauge(split, 0, 100, st->load_percent, "%");
    lv_obj_set_flex_grow(st->dash_gauge, 1);

    st->dash_line_chart = view::chart(split, blusys::ui::chart_type::line,
                                      static_cast<std::int32_t>(app_state::history_size), 0, 100);
    lv_obj_set_flex_grow(st->dash_line_chart, 2);
    st->line_series = blusys::ui::chart_add_series(st->dash_line_chart,
                                                   blusys::ui::theme().color_accent);

    auto *zones = view::card(page.content, "Zone mix", card_pad);
    lv_obj_set_width(zones, LV_PCT(100));
    st->dash_bar_chart =
        view::chart(zones, blusys::ui::chart_type::bar,
                    static_cast<std::int32_t>(app_state::zone_count), 0, 100);
    lv_obj_set_width(st->dash_bar_chart, LV_PCT(100));
    lv_obj_set_height(st->dash_bar_chart, bar_h);
    st->bar_series = blusys::ui::chart_add_series(st->dash_bar_chart,
                                                  blusys::ui::theme().color_primary);

    auto *mode_row = view::row(page.content);
    lv_obj_set_width(mode_row, LV_PCT(100));
    view::label(mode_row, "Profile", blusys::ui::theme().font_body_sm);
    view::button(mode_row, "Eco", action{.tag = action_tag::set_mode, .value = 0}, &ctx,
                 blusys::ui::button_variant::secondary);
    view::button(mode_row, "Balanced", action{.tag = action_tag::set_mode, .value = 1}, &ctx,
                 blusys::ui::button_variant::secondary);
    view::button(mode_row, "Boost", action{.tag = action_tag::set_mode, .value = 2}, &ctx,
                 blusys::ui::button_variant::secondary);

    auto *ctrl = view::row(page.content);
    lv_obj_set_width(ctrl, LV_PCT(100));
    view::button(ctrl, "−", action{.tag = action_tag::nudge_load, .value = -6}, &ctx,
                 blusys::ui::button_variant::secondary);
    view::button(ctrl, "+", action{.tag = action_tag::nudge_load, .value = 6}, &ctx,
                 blusys::ui::button_variant::secondary);
    view::label(ctrl, "Target", blusys::ui::theme().font_body_sm);
    view::slider(ctrl, 0, 100, st->target_setpoint, &make_set_target, &ctx);

    if (group_out != nullptr) {
        *group_out = page.group;
    }
    return ctx.shell() != nullptr ? page.content : page.screen;
}

lv_obj_t *create_about(blusys::app::app_ctx &ctx, const void * /*params*/, lv_group_t **group_out)
{
    const blusys::app::screens::about_extra_field extras[] = {
        {.key = "Archetype", .value = "interactive dashboard"},
        {.key = "Profile", .value = interactive_dashboard::system::dashboard_profile_label_for_build()},
        {.key = "Identity", .value = "expressive_dark"},
        {.key = "Focus", .value = "gauges · trends · zones"},
    };

    const blusys::app::screens::about_screen_config config{
        .product_name     = "Ops Dashboard",
        .firmware_version = interactive_dashboard::system::dashboard_build_version_for_build(),
        .hardware_version = interactive_dashboard::system::dashboard_hardware_label_for_build(),
        .serial_number    = "DASH-0001",
        .build_date       = __DATE__,
        .extras           = extras,
        .extra_count      = sizeof(extras) / sizeof(extras[0]),
    };

    return blusys::app::screens::about_screen_create(ctx, &config, group_out);
}

void register_all_screens(view::screen_router *router, blusys::app::app_ctx &ctx)
{
    static const view::screen_registration k_routes[] = {
        view::screen_row(route_overview, &create_overview, nullptr, &on_overview_show,
                         &on_overview_hide),
        view::screen_row(route_about, &create_about, nullptr, &on_about_show, nullptr),
    };

    router->register_screens(&ctx, k_routes, sizeof(k_routes) / sizeof(k_routes[0]));
}

}  // namespace

void on_init(blusys::app::app_ctx &ctx, app_state &state)
{
    (void)state;

    if (ctx.shell() != nullptr) {
        const view::shell_tab_item tabs[] = {
            {.label = "Overview", .route_id = route_overview},
            {.label = "About", .route_id = route_about},
        };
        view::shell_set_tabs(*ctx.shell(), tabs, sizeof(tabs) / sizeof(tabs[0]), &ctx);

        if (lv_obj_t *surface = view::shell_status_surface(*ctx.shell()); surface != nullptr) {
            state.shell_badge =
                view::status_badge(surface, "Init", blusys::ui::badge_level::warning);
            state.shell_detail =
                view::label(surface, "Balanced  sp55  Q3", blusys::ui::theme().font_body_sm);
        }
    }

    auto *router = ctx.screen_router();
    if (router == nullptr) {
        return;
    }

    register_all_screens(router, ctx);

    ctx.navigate_to(route_overview);
}

}  // namespace interactive_dashboard::ui
