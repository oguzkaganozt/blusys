// Optional local mono UI for the edge node reference (SSD1306).
// When disabled in Kconfig, on_init only logs — same entry as headless.

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "ui/app_ui.hpp"

#include "blusys/app/view/action_widgets.hpp"
#include "blusys/app/view/page.hpp"
#include "blusys/log.h"

#include "lvgl.h"

#include <cstdio>

#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#endif

namespace edge_node::ui {

namespace view = blusys::app::view;

namespace {

constexpr std::uint32_t kRouteStatus = 1;

app_state *g_state = nullptr;

lv_obj_t *g_lbl_phase = nullptr;
lv_obj_t *g_lbl_temp  = nullptr;
lv_obj_t *g_lbl_hum   = nullptr;
lv_obj_t *g_lbl_tel   = nullptr;

void refresh_timer_cb(lv_timer_t *timer)
{
    auto *st = static_cast<app_state *>(lv_timer_get_user_data(timer));
    if (st == nullptr) {
        return;
    }
    char buf[48]{};

    if (g_lbl_phase != nullptr) {
        lv_label_set_text(g_lbl_phase, op_state_name(st->phase));
    }
    if (g_lbl_temp != nullptr) {
        std::snprintf(buf, sizeof(buf), "%.1f C", static_cast<double>(st->temperature));
        lv_label_set_text(g_lbl_temp, buf);
    }
    if (g_lbl_hum != nullptr) {
        std::snprintf(buf, sizeof(buf), "%.0f %%", static_cast<double>(st->humidity));
        lv_label_set_text(g_lbl_hum, buf);
    }
    if (g_lbl_tel != nullptr) {
        std::snprintf(buf, sizeof(buf), "tel ok:%u", static_cast<unsigned>(st->delivered));
        lv_label_set_text(g_lbl_tel, buf);
    }
}

lv_obj_t *create_status(blusys::app::app_ctx & /*ctx*/, const void * /*params*/,
                        lv_group_t **group_out)
{
    view::page page = view::page_create({.scrollable = true});

    view::label(page.content, "Edge");
    g_lbl_phase = view::label(page.content, op_state_name(g_state->phase));
    g_lbl_temp  = view::label(page.content, "—");
    g_lbl_hum   = view::label(page.content, "—");
    g_lbl_tel   = view::label(page.content, "tel —");

    if (group_out != nullptr) {
        *group_out = page.group;
    }
    return page.screen;
}

}  // namespace

void on_init(blusys::app::app_ctx &ctx, app_state &state)
{
    BLUSYS_LOGI("edge_node", "edge node initialized — entering operational loop");

    g_state = &state;

#if defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_EDGE_NODE_LOCAL_UI) && \
    CONFIG_BLUSYS_EDGE_NODE_LOCAL_UI
    auto *router = ctx.screen_router();
    if (router == nullptr) {
        return;
    }

    router->register_screen(kRouteStatus, &create_status, nullptr, {});
    ctx.navigate_to(kRouteStatus);

    lv_timer_create(refresh_timer_cb, 400, &state);
#endif
}

}  // namespace edge_node::ui

#endif  // BLUSYS_FRAMEWORK_HAS_UI
