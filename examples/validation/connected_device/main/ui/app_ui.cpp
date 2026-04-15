#include "ui/app_ui.hpp"

#include "core/app_logic.hpp"

#include "blusys/framework/ui/binding/action_widgets.hpp"
#include "blusys/framework/ui/composition/page.hpp"
#include "blusys/framework/ui/widgets/slider.hpp"
#include "blusys/hal/log.h"

namespace connected_device::ui {

void on_init(blusys::app_ctx &ctx, blusys::app_services &svc, State &state)
{
    (void)svc;
    using Tag = connected_device::Tag;
    namespace view = blusys;

    auto p = view::page_create();

    view::title(p.content, "Connected Device");
    view::divider(p.content);

    state.status_lbl = view::label(p.content, "Connecting...");
    state.ip_lbl     = view::label(p.content, "---");

    view::divider(p.content);
    view::label(p.content, "Brightness");

    state.slider = blusys::slider_create(p.content, {
        .min     = 0,
        .max     = 100,
        .initial = state.brightness,
    });

    auto *btn_row = view::row(p.content);
    view::button(btn_row, "-", Action{.tag = Tag::brightness_down}, &ctx,
                 blusys::button_variant::secondary);
    view::button(btn_row, "+", Action{.tag = Tag::brightness_up}, &ctx,
                 blusys::button_variant::secondary);
    view::button(btn_row, "OK", Action{.tag = Tag::confirm}, &ctx);

    view::page_load(p);

    BLUSYS_LOGI(kTag, "UI initialized");
}

}  // namespace connected_device::ui
