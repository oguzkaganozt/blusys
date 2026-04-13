#include "ui/app_ui.hpp"

#include "blusys/app/view/page.hpp"
#include "blusys/log.h"

namespace handheld_bluetooth::ui {

void on_init(blusys::app::app_ctx &ctx, blusys::app::app_services &svc, State &state)
{
    (void)ctx;
    (void)svc;
    namespace view = blusys::app::view;

    auto p = view::page_create();

    view::title(p.content, "Bluetooth (handheld)");
    view::divider(p.content);

    state.status_lbl = view::label(p.content, "Starting…");
    state.advert_lbl = view::label(p.content, "---");

    view::divider(p.content);
    view::label(p.content, "Encoder: Enter = ping");

    view::page_load(p);

    BLUSYS_LOGI(kTag, "UI initialized");
}

}  // namespace handheld_bluetooth::ui
