#include "ui/app_ui.hpp"

#include "core/app_logic.hpp"

#include "blusys/framework/ui/binding/action_widgets.hpp"
#include "blusys/framework/ui/composition/overlay_manager.hpp"
#include "blusys/framework/ui/composition/page.hpp"
#include "blusys/framework/ui/widgets/slider.hpp"
#include "blusys/hal/log.h"

namespace framework_device_basic::ui {

void on_init(blusys::app_ctx &ctx, blusys::app_fx &fx, AppState &state)
{
    (void)fx;
    using Tag = framework_device_basic::Tag;
    namespace view = blusys;

    auto p = view::page_create();

    view::title(p.content, "Blusys Volume Control");
    view::divider(p.content);

    state.mute_label = view::label(p.content, "Volume");

    state.slider = blusys::slider_create(p.content, {
        .min     = 0,
        .max     = 100,
        .initial = state.volume,
    });

    auto *btn_row = view::row(p.content);

    view::button(btn_row, "Down", Action{.tag = Tag::volume_down}, &ctx,
                 blusys::button_variant::secondary);
    view::button(btn_row, "Up", Action{.tag = Tag::volume_up}, &ctx,
                 blusys::button_variant::secondary);
    view::button(btn_row, "Mute", Action{.tag = Tag::toggle_mute}, &ctx,
                 blusys::button_variant::ghost);
    view::button(btn_row, "OK", Action{.tag = Tag::confirm}, &ctx);

    view::overlay_create(p.screen, 1,
                         {.text = "Settings saved", .duration_ms = 1500},
                         *ctx.fx().nav.overlay_manager());

    view::page_load(p);

    BLUSYS_LOGI(kTag, "volume control app initialized — volume=%ld",
                static_cast<long>(state.volume));
}

}  // namespace framework_device_basic::ui
