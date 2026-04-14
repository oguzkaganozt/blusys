#include "core/app_logic.hpp"
#include "ui/app_ui.hpp"

#include "blusys/framework/capabilities/bluetooth.hpp"
#include "blusys/framework/capabilities/storage.hpp"
#include "blusys/framework/platform/profiles/st7735.hpp"

namespace {

static blusys::app::storage_capability storage{{
    .init_nvs = true,
}};

static blusys::app::bluetooth_capability bluetooth{{
    .device_name    = "BlusysHandheldBT",
    .auto_advertise = true,
}};

static blusys::app::capability_list capabilities{&storage, &bluetooth};

static const blusys::app::app_spec<handheld_bluetooth::State, handheld_bluetooth::Action> spec{
    .initial_state  = {},
    .update         = handheld_bluetooth::update,
    .on_init        = handheld_bluetooth::ui::on_init,
    .map_intent     = handheld_bluetooth::map_intent,
    .capability_event_discriminant =
        static_cast<std::uint32_t>(handheld_bluetooth::Tag::capability_event),
    .capabilities   = &capabilities,
};

}  // namespace

#if defined(ESP_PLATFORM)
BLUSYS_APP_MAIN_DEVICE(spec, blusys::app::profiles::st7735_160x128())
#else
// Host / SDL (see host/CMakeLists.txt) — `BLUSYS_APP_MAIN_DEVICE` is device-only.
BLUSYS_APP_MAIN_HOST(spec)
#endif
