#include "core/app_logic.hpp"
#include "ui/app_ui.hpp"
#include "blusys/blusys.hpp"


namespace {

static const blusys::device_profile kProfile = blusys::platform::st7735_160x128();

static const blusys::app_spec<framework_device_basic::AppState, framework_device_basic::Action> spec{
    .initial_state  = {.volume = 50, .muted = false},
    .update         = framework_device_basic::update,
    .on_init        = framework_device_basic::ui::on_init,
    .on_tick        = nullptr,
    .on_event       = framework_device_basic::on_event,
    .tick_period_ms = 10,
    .profile        = &kProfile,
};

}  // namespace

BLUSYS_APP(spec)
