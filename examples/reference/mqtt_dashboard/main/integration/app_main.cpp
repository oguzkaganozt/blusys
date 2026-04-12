#include "core/app_logic.hpp"
#include "ui/app_ui.hpp"

#include "blusys/app/build_profile.hpp"
#include "blusys/app/capabilities/mqtt_host.hpp"
#include "blusys/app/layout_surface.hpp"
#include "blusys/app/theme_presets.hpp"

#include <cstdint>

#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#include "blusys/app/profiles/ili9341.hpp"
#include "blusys/app/profiles/ili9488.hpp"
#endif

#ifndef BLUSYS_MQTT_DASH_BROKER
#define BLUSYS_MQTT_DASH_BROKER "mqtt://test.mosquitto.org"
#endif

namespace mqtt_dashboard::system {

blusys::app::device_profile mqtt_dashboard_device_profile_for_build()
{
#if defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_IDASH_DISPLAY_PROFILE_ILI9488) && \
    CONFIG_BLUSYS_IDASH_DISPLAY_PROFILE_ILI9488
    return blusys::app::profiles::ili9488_480x320();
#elif defined(ESP_PLATFORM)
    return blusys::app::profiles::ili9341_320x240();
#else
    blusys::app::device_profile p{};
#if defined(BLUSYS_MQTT_DASH_HOST_DISPLAY_PROFILE) && (BLUSYS_MQTT_DASH_HOST_DISPLAY_PROFILE == 1)
    p.lcd.width          = 480;
    p.lcd.height         = 320;
#else
    p.lcd.width          = 320;
    p.lcd.height         = 240;
#endif
    p.lcd.bits_per_pixel = 16;
    p.ui.panel_kind      = BLUSYS_UI_PANEL_KIND_RGB565;
    return p;
#endif
}

const char *mqtt_dashboard_profile_label_for_build()
{
#if defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_IDASH_DISPLAY_PROFILE_ILI9488) && \
    CONFIG_BLUSYS_IDASH_DISPLAY_PROFILE_ILI9488
    return "ILI9488 480x320";
#elif defined(ESP_PLATFORM)
    return "ILI9341 320x240";
#elif defined(BLUSYS_MQTT_DASH_HOST_DISPLAY_PROFILE) && (BLUSYS_MQTT_DASH_HOST_DISPLAY_PROFILE == 1)
    return "Host SDL 480x320";
#else
    return "Host SDL 320x240";
#endif
}

const char *mqtt_dashboard_hardware_label_for_build()
{
#if defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_IDASH_DISPLAY_PROFILE_ILI9488) && \
    CONFIG_BLUSYS_IDASH_DISPLAY_PROFILE_ILI9488
    return "ILI9488 reference";
#elif defined(ESP_PLATFORM)
    return "ILI9341 reference";
#else
    return "Host simulation";
#endif
}

const char *mqtt_dashboard_version_for_build()
{
    return blusys::app::build_profile::version_string();
}

namespace {

const blusys::app::view::shell_config mqtt_shell_for_profile()
{
    const auto prof   = mqtt_dashboard_device_profile_for_build();
    const auto chrome = blusys::app::layout::shell_chrome_for(prof);
    blusys::app::view::shell_config c{};
    c.header.enabled = true;
    c.header.title   = "MQTT";
    c.status.enabled = chrome.status_enabled;
    c.tabs.enabled   = chrome.tabs_enabled;

    const bool compact_chrome =
        (prof.lcd.height <= 280) || (prof.lcd.width <= 360 && prof.lcd.height <= 300);
    if (compact_chrome) {
        c.header.height = 30;
        c.status.height = 16;
        c.tabs.height   = 28;
    }
    return c;
}

static const blusys::app::view::shell_config kShellConfig = mqtt_shell_for_profile();

#if !defined(ESP_PLATFORM)
static blusys::app::mqtt_host_config make_mqtt_cfg()
{
    blusys::app::mqtt_host_config c{};
    c.broker_url                        = BLUSYS_MQTT_DASH_BROKER;
    c.client_id                         = nullptr;
    c.subscribe_topics[0]               = "blusys/demo/#";
    c.keepalive_sec                     = 60;
    c.qos                               = 1;
    return c;
}

static blusys::app::mqtt_host_capability g_mqtt_host(make_mqtt_cfg());
#endif

static blusys::app::diagnostics_capability diagnostics{{
    .enable_console       = false,
    .snapshot_interval_ms = 2000,
}};

#if !defined(ESP_PLATFORM)
static blusys::app::capability_list capabilities{&g_mqtt_host, &diagnostics};
#else
static blusys::app::capability_list capabilities{&diagnostics};
#endif

static const blusys::app::app_spec<app_state, action> spec{
    .initial_state  = {},
    .update         = update,
    .on_init        = ui::on_init,
    .on_tick        = nullptr,
    .map_intent     = map_intent,
    .capability_event_discriminant =
        static_cast<std::uint32_t>(action_tag::capability_event),
    .tick_period_ms = 50,
    .capabilities   = &capabilities,
    .theme          = &blusys::app::presets::expressive_dark(),
    .shell          = &kShellConfig,
};

}  // namespace

}  // namespace mqtt_dashboard::system

#ifdef ESP_PLATFORM
BLUSYS_APP_MAIN_DEVICE(mqtt_dashboard::system::spec,
                       mqtt_dashboard::system::mqtt_dashboard_device_profile_for_build())
#else
BLUSYS_APP_MAIN_HOST_PROFILE(
    mqtt_dashboard::system::spec,
    (blusys::app::host_profile{
#if defined(BLUSYS_MQTT_DASH_HOST_DISPLAY_PROFILE) && (BLUSYS_MQTT_DASH_HOST_DISPLAY_PROFILE == 1)
        .hor_res = 480,
        .ver_res = 320,
#else
        .hor_res = 320,
        .ver_res = 240,
#endif
        .title = "MQTT dashboard",
    }))
#endif
