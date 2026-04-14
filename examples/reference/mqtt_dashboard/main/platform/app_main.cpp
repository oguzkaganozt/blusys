#include "core/app_logic.hpp"
#include "ui/app_ui.hpp"

#include "blusys/framework/platform/auto.hpp"
#include "blusys/framework/platform/auto_shell.hpp"
#include "blusys/framework/platform/build.hpp"
#include "blusys/framework/capabilities/connectivity.hpp"
#include "blusys/framework/capabilities/mqtt_host.hpp"
#include "blusys/framework/platform/profile.hpp"
#include "blusys/framework/ui/style/presets.hpp"
#include "blusys/framework/ui/composition/shell.hpp"

#ifdef ESP_PLATFORM
#include "integration/mqtt_esp_capability.hpp"
#endif

#include <cstdint>

#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#endif

#ifndef BLUSYS_MQTT_DASH_BROKER
#define BLUSYS_MQTT_DASH_BROKER "mqtt://test.mosquitto.org"
#endif

namespace mqtt_dashboard::system {

const char *mqtt_dashboard_profile_label_for_build()
{
    return blusys::app::dashboard_profile_label();
}

const char *mqtt_dashboard_hardware_label_for_build()
{
    return blusys::app::dashboard_hardware_label();
}

const char *mqtt_dashboard_version_for_build()
{
    return blusys::app::build_profile::version_string();
}

namespace {

static const char kShellTitle[] = "MQTT";

[[nodiscard]] static blusys::app::view::shell_config make_mqtt_shell()
{
    const blusys::app::device_profile p = blusys::app::auto_profile_dashboard();
    if (p.ui.panel_kind == BLUSYS_UI_PANEL_KIND_MONO_PAGE) {
        blusys::app::view::shell_config c{};
        c.header.enabled = true;
        c.header.title   = kShellTitle;
        c.header.height  = 12;
        c.status.enabled = false;
        c.tabs.enabled   = true;
        c.tabs.height    = 14;
        return c;
    }
    if (p.lcd.width <= 200u && p.lcd.height <= 160u) {
        blusys::app::view::shell_config c{};
        c.header.enabled = true;
        c.header.title   = kShellTitle;
        c.header.height  = 22;
        c.status.enabled = false;
        c.tabs.enabled   = true;
        c.tabs.height    = 22;
        return c;
    }
    return blusys::app::auto_shell_adaptive(p, kShellTitle);
}

static const blusys::app::view::shell_config kShellConfig = make_mqtt_shell();

#if defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_SSD1306) && \
    CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_SSD1306
static const blusys::ui::theme_tokens *const k_dashboard_theme = &blusys::app::presets::oled();
#else
static const blusys::ui::theme_tokens *const k_dashboard_theme =
    &blusys::app::presets::expressive_dark();
#endif

#if !defined(ESP_PLATFORM)
static blusys::app::mqtt_host_config make_mqtt_cfg()
{
    blusys::app::mqtt_host_config c{};
    c.broker_url           = BLUSYS_MQTT_DASH_BROKER;
    c.client_id            = nullptr;
    c.subscribe_topics[0]    = "blusys/demo/#";
    c.keepalive_sec        = 60;
    c.qos                  = 1;
    return c;
}

static blusys::app::mqtt_host_capability g_mqtt_host(make_mqtt_cfg());
#endif

static blusys::app::diagnostics_capability diagnostics{{
    .enable_console       = false,
    .snapshot_interval_ms = 2000,
}};

#ifdef ESP_PLATFORM
static blusys::app::connectivity_capability connectivity{{
    .wifi_ssid            = CONFIG_WIFI_SSID,
    .wifi_password        = CONFIG_WIFI_PASSWORD,
    .auto_reconnect       = true,
    .reconnect_delay_ms   = 1000,
    .max_retries          = -1,
    .connect_timeout_ms   = CONFIG_WIFI_CONNECT_TIMEOUT_MS,
    .sntp_server          = nullptr,
}};

static mqtt_esp_capability g_mqtt_esp(
    mqtt_esp_config{.broker_url             = CONFIG_MQTT_BROKER_URL,
                    .mqtt_connect_timeout_ms = CONFIG_MQTT_CONNECT_TIMEOUT_MS,
                    .subscribe_pattern       = "blusys/demo/#",
                    .qos                     = 1});

static blusys::app::capability_list capabilities{&connectivity, &g_mqtt_esp, &diagnostics};
#else
static blusys::app::capability_list capabilities{&g_mqtt_host, &diagnostics};
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
    .theme          = k_dashboard_theme,
    .shell          = &kShellConfig,
};

}  // namespace

#ifdef ESP_PLATFORM
[[nodiscard]] blusys::app::device_profile dashboard_device_profile()
{
    blusys::app::device_profile p = blusys::app::auto_profile_dashboard();
#if defined(CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_SSD1306) && \
    CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_SSD1306 && defined(CONFIG_IDF_TARGET_ESP32C3) && \
    CONFIG_IDF_TARGET_ESP32C3
    p.lcd.i2c.sda_pin = 8;
    p.lcd.i2c.scl_pin = 9;
#elif defined(CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_SSD1306) && \
    CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_SSD1306 && defined(CONFIG_IDF_TARGET_ESP32S3) && \
    CONFIG_IDF_TARGET_ESP32S3
    /* Same as profiles/ssd1306.hpp defaults for S3; explicit for this example’s wiring. */
    p.lcd.i2c.sda_pin = 1;
    p.lcd.i2c.scl_pin = 2;
#endif
    return p;
}
#endif

}  // namespace mqtt_dashboard::system

#ifdef ESP_PLATFORM
BLUSYS_APP_MAIN_DEVICE(mqtt_dashboard::system::spec,
                       mqtt_dashboard::system::dashboard_device_profile())
#else
BLUSYS_APP_DASHBOARD(mqtt_dashboard::system::spec, "MQTT dashboard")
#endif
