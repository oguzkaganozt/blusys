#include "core/app_logic.hpp"
#include "ui/app_ui.hpp"

#include "blusys/framework/platform/profiles/st7735.hpp"

namespace {

static const blusys::app_spec<framework_device_basic::AppState, framework_device_basic::Action> spec{
    .initial_state  = {.volume = 50, .muted = false},
    .update         = framework_device_basic::update,
    .on_init        = framework_device_basic::ui::on_init,
    .on_tick        = nullptr,
    .on_event       = framework_device_basic::on_event,
    .tick_period_ms = 10,
};

}  // namespace

BLUSYS_APP_MAIN_DEVICE(spec, blusys::platform::st7735_160x128())
