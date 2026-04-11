#include "core/app_logic.hpp"
#include "ui/app_ui.hpp"

#include "blusys/app/layout_surface.hpp"
#include "blusys/app/theme_presets.hpp"
#include "blusys/version.h"

#include <cstdint>

#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#include "blusys/app/profiles/st7735.hpp"
#include "blusys/app/profiles/st7789.hpp"
#endif

namespace interactive_controller::system {

blusys::app::device_profile controller_device_profile_for_build()
{
#if defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_IC_DISPLAY_PROFILE_ST7789) && \
    CONFIG_BLUSYS_IC_DISPLAY_PROFILE_ST7789
    return blusys::app::profiles::st7789_320x240();
#elif defined(ESP_PLATFORM)
    return blusys::app::profiles::st7735_160x128();
#else
    // Host SDL: dimensions follow BLUSYS_IC_HOST_DISPLAY_PROFILE (see host/CMakeLists.txt).
    blusys::app::device_profile p{};
#if defined(BLUSYS_IC_HOST_DISPLAY_PROFILE) && (BLUSYS_IC_HOST_DISPLAY_PROFILE == 1)
    p.lcd.width          = 320;
    p.lcd.height         = 240;
#else
    p.lcd.width          = 240;
    p.lcd.height         = 240;
#endif
    p.lcd.bits_per_pixel = 16;
    p.ui.panel_kind      = BLUSYS_UI_PANEL_KIND_RGB565;
    return p;
#endif
}

const char *controller_profile_label_for_build()
{
#if defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_IC_DISPLAY_PROFILE_ST7789) && \
    CONFIG_BLUSYS_IC_DISPLAY_PROFILE_ST7789
    return "ST7789 320x240";
#elif defined(ESP_PLATFORM)
    return "ST7735 160x128";
#elif defined(BLUSYS_IC_HOST_DISPLAY_PROFILE) && (BLUSYS_IC_HOST_DISPLAY_PROFILE == 1)
    return "Host SDL 320x240";
#else
    return "Host SDL 240x240";
#endif
}

const char *controller_hardware_label_for_build()
{
#if defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_IC_DISPLAY_PROFILE_ST7789) && \
    CONFIG_BLUSYS_IC_DISPLAY_PROFILE_ST7789
    return "ST7789 reference";
#elif defined(ESP_PLATFORM)
    return "ST7735 reference";
#else
    return "Host simulation";
#endif
}

const char *controller_build_version_for_build()
{
#ifdef PROJECT_VER
    return PROJECT_VER;
#elif defined(BLUSYS_APP_BUILD_VERSION)
    return BLUSYS_APP_BUILD_VERSION;
#else
    return BLUSYS_VERSION_STRING;
#endif
}

namespace {

const blusys::app::view::shell_config controller_shell_for_profile()
{
    const auto            prof = controller_device_profile_for_build();
    const auto            chrome = blusys::app::layout::shell_chrome_for(prof);
    blusys::app::view::shell_config c{};
    c.header.enabled = true;
    c.header.title   = "Controller";
    c.status.enabled = chrome.status_enabled;
    c.tabs.enabled   = chrome.tabs_enabled;
    return c;
}

static const blusys::app::view::shell_config kShellConfig = controller_shell_for_profile();

bool map_event(std::uint32_t id, std::uint32_t /*code*/, const void * /*payload*/, action *out)
{
    switch (id) {
    case static_cast<std::uint32_t>(blusys::app::storage_event::capability_ready):
        *out = action{.tag = action_tag::sync_storage};
        return true;
    case static_cast<std::uint32_t>(blusys::app::provisioning_event::started):
    case static_cast<std::uint32_t>(blusys::app::provisioning_event::credentials_received):
    case static_cast<std::uint32_t>(blusys::app::provisioning_event::success):
    case static_cast<std::uint32_t>(blusys::app::provisioning_event::failed):
    case static_cast<std::uint32_t>(blusys::app::provisioning_event::already_provisioned):
    case static_cast<std::uint32_t>(blusys::app::provisioning_event::reset_complete):
    case static_cast<std::uint32_t>(blusys::app::provisioning_event::capability_ready):
        *out = action{.tag = action_tag::sync_provisioning};
        return true;
    default:
        return false;
    }
}

static blusys::app::storage_capability storage{{
    .init_nvs        = true,
    .spiffs_base_path = "/controller",
}};

static blusys::app::provisioning_capability provisioning{{
    .service_name = "pulse-ctrl",
    .pop = "123456",
}};

static blusys::app::capability_list capabilities{&storage, &provisioning};

static const blusys::app::app_spec<app_state, action> spec{
    .initial_state = {},
    .update = update,
    .on_init = ui::on_init,
    .map_intent = map_intent,
    .map_event = map_event,
    .capabilities = &capabilities,
    .theme = &blusys::app::presets::expressive_dark(),
    .shell = &kShellConfig,
};

}  // namespace

}  // namespace interactive_controller::system

#ifdef ESP_PLATFORM
BLUSYS_APP_MAIN_DEVICE(interactive_controller::system::spec,
                       interactive_controller::system::controller_device_profile_for_build())
#else
BLUSYS_APP_MAIN_HOST_PROFILE(
    interactive_controller::system::spec,
    (blusys::app::host_profile{
#if defined(BLUSYS_IC_HOST_DISPLAY_PROFILE) && (BLUSYS_IC_HOST_DISPLAY_PROFILE == 1)
        .hor_res = 320,
        .ver_res = 240,
#else
        .hor_res = 240,
        .ver_res = 240,
#endif
        .title = "Pulse Controller",
    }))
#endif
