#include "ui/app_ui.hpp"

#include "core/app_logic.hpp"

#include "blusys/app/view/action_widgets.hpp"
#include "blusys/app/view/page.hpp"
#include "blusys/framework/ui/widgets/slider/slider.hpp"
#include "blusys/log.h"

namespace connected_device::ui {

void on_init(blusys::app::app_ctx &ctx, State &state)
{
    using Tag = connected_device::Tag;
    namespace view = blusys::app::view;

    auto p = view::page_create();

    view::title(p.content, "Connected Device");
    view::divider(p.content);

    state.status_lbl = view::label(p.content, "Connecting...");
    state.ip_lbl     = view::label(p.content, "---");

    view::divider(p.content);
    view::label(p.content, "Brightness");

    state.slider = blusys::ui::slider_create(p.content, {
        .min     = 0,
        .max     = 100,
        .initial = state.brightness,
    });

    auto *btn_row = view::row(p.content);
    view::button(btn_row, "-", Action{.tag = Tag::brightness_down}, &ctx,
                 blusys::ui::button_variant::secondary);
    view::button(btn_row, "+", Action{.tag = Tag::brightness_up}, &ctx,
                 blusys::ui::button_variant::secondary);
    view::button(btn_row, "OK", Action{.tag = Tag::confirm}, &ctx);

    view::page_load(p);

    BLUSYS_LOGI(kTag, "UI initialized");
}

}  // namespace connected_device::ui
