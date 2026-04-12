#include "core/app_logic.hpp"
#include "ui/app_ui.hpp"

#include "blusys/app/auto_profile.hpp"
#include "blusys/app/auto_shell.hpp"
#include "blusys/app/build_profile.hpp"
#include "blusys/app/capabilities/mqtt_host.hpp"
#include "blusys/app/theme_presets.hpp"

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

static const blusys::app::view::shell_config kShellConfig =
    blusys::app::auto_shell_adaptive(blusys::app::auto_profile_dashboard(), "MQTT");

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

BLUSYS_APP_DASHBOARD(mqtt_dashboard::system::spec, "MQTT dashboard")
