// Optional local mono UI for the headless telemetry quickstart (SSD1306).
// When disabled in Kconfig, on_init only logs — same entry as headless.

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "ui/app_ui.hpp"

#include "blusys/framework/ui/binding/action_widgets.hpp"
#include "blusys/framework/ui/binding/bindings.hpp"
#include "blusys/framework/ui/composition/page.hpp"
#include "blusys/hal/log.h"

#include "lvgl.h"

#include <cstdio>

#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#endif

namespace headless_telemetry::ui {

namespace view = blusys;

namespace {

#if defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_HEADLESS_TELEMETRY_LOCAL_UI) && \
    CONFIG_BLUSYS_HEADLESS_TELEMETRY_LOCAL_UI

void set_label_text(lv_obj_t *label, const char *text)
{
    if (label != nullptr && text != nullptr) {
        view::set_text(label, text);
    }
}

enum class route : std::uint32_t {
    status = 1,
};

using headless_telemetry::app_state;
using headless_telemetry::op_state_name;

void refresh_timer_cb(lv_timer_t *timer)
{
    auto *st = static_cast<app_state *>(lv_timer_get_user_data(timer));
    if (st == nullptr) {
        return;
    }
    char buf[48]{};

    set_label_text(st->mono_ui.phase_lbl, op_state_name(st->phase));
    if (st->mono_ui.temp_lbl != nullptr) {
        std::snprintf(buf, sizeof(buf), "%.1f C", static_cast<double>(st->temperature));
        set_label_text(st->mono_ui.temp_lbl, buf);
    }
    if (st->mono_ui.hum_lbl != nullptr) {
        std::snprintf(buf, sizeof(buf), "%.0f %%", static_cast<double>(st->humidity));
        set_label_text(st->mono_ui.hum_lbl, buf);
    }
    if (st->mono_ui.tel_lbl != nullptr) {
        std::snprintf(buf, sizeof(buf), "tel ok:%u", static_cast<unsigned>(st->delivered));
        set_label_text(st->mono_ui.tel_lbl, buf);
    }
}

lv_obj_t *create_status(blusys::app_ctx &ctx, const void * /*params*/,
                        lv_group_t **group_out)
{
    auto *st = ctx.product_state<app_state>();
    if (st == nullptr) {
        return nullptr;
    }

    view::page page = view::page_create({.scrollable = true});

    view::label(page.content, "Telemetry");
    st->mono_ui.phase_lbl = view::label(page.content, op_state_name(st->phase));
    st->mono_ui.temp_lbl  = view::label(page.content, "—");
    st->mono_ui.hum_lbl   = view::label(page.content, "—");
    st->mono_ui.tel_lbl   = view::label(page.content, "tel —");

    if (group_out != nullptr) {
        *group_out = page.group;
    }
    return page.screen;
}

#endif

}  // namespace

void on_init(blusys::app_ctx &ctx, blusys::app_fx &fx, app_state &state)
{
    (void)fx;
    BLUSYS_LOGI("headless_telemetry",
                "headless telemetry reference initialized — entering operational loop");

#if defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_HEADLESS_TELEMETRY_LOCAL_UI) && \
    CONFIG_BLUSYS_HEADLESS_TELEMETRY_LOCAL_UI
    auto *router = ctx.fx().nav.screen_router();
    if (router == nullptr) {
        return;
    }

    static const view::screen_registration kRoutes[] = {
        view::screen_row(route::status, &create_status, nullptr, nullptr, nullptr),
    };
    router->register_screens(&ctx, kRoutes, sizeof(kRoutes) / sizeof(kRoutes[0]));
    ctx.fx().nav.to(route::status);

    lv_timer_create(refresh_timer_cb, 400, &state);
#endif
}

}  // namespace headless_telemetry::ui

#endif  // BLUSYS_FRAMEWORK_HAS_UI
