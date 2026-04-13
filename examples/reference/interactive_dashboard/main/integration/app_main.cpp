#include "core/app_logic.hpp"
#include "ui/app_ui.hpp"

#include "blusys/app/auto_profile.hpp"
#include "blusys/app/auto_shell.hpp"
#include "blusys/app/build_profile.hpp"
#include "blusys/app/platform_profile.hpp"
#include "blusys/app/theme_presets.hpp"
#include "blusys/app/view/shell.hpp"

#include <cstdint>

#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#endif

namespace interactive_dashboard::system {

const char *dashboard_profile_label_for_build()
{
    return blusys::app::dashboard_profile_label();
}

const char *dashboard_hardware_label_for_build()
{
    return blusys::app::dashboard_hardware_label();
}

const char *dashboard_build_version_for_build()
{
    return blusys::app::build_profile::version_string();
}

namespace {

static const char kShellTitle[] = "Dashboard";

// ST7735-class (≤200×160): drop status bar + tighten header/tabs. SSD1306 128×64:
// minimal chrome so Overview + About fit; see layout_surface tiny_mono.
[[nodiscard]] static blusys::app::view::shell_config make_dashboard_shell()
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

static const blusys::app::view::shell_config kShellConfig = make_dashboard_shell();

#if defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_SSD1306) && \
    CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_SSD1306
static const blusys::ui::theme_tokens *const k_dashboard_theme = &blusys::app::presets::oled();
#else
static const blusys::ui::theme_tokens *const k_dashboard_theme =
    &blusys::app::presets::expressive_dark();
#endif

static blusys::app::diagnostics_capability diagnostics{{
    .enable_console        = false,
    .snapshot_interval_ms  = 1000,
}};

static blusys::app::storage_capability storage{{
    .init_nvs         = true,
    .spiffs_base_path = "/dashbd",
}};

static blusys::app::capability_list capabilities{&diagnostics, &storage};

static const blusys::app::app_spec<app_state, action> spec{
    .initial_state    = {},
    .update           = update,
    .on_init          = ui::on_init,
    .on_tick          = on_tick,
    .map_intent       = map_intent,
    .capability_event_discriminant =
        static_cast<std::uint32_t>(action_tag::capability_event),
    .tick_period_ms   = 400,
    .capabilities     = &capabilities,
    .theme            = k_dashboard_theme,
    .shell            = &kShellConfig,
};

}  // namespace

// Same logical profile as auto_profile_dashboard(); ESP32-C3 + SSD1306 uses SDA=8 / SCL=9 for this board.
[[nodiscard]] blusys::app::device_profile dashboard_device_profile()
{
    blusys::app::device_profile p = blusys::app::auto_profile_dashboard();
#if defined(ESP_PLATFORM) && defined(CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_SSD1306) && \
    CONFIG_BLUSYS_DASHBOARD_DISPLAY_PROFILE_SSD1306 && defined(CONFIG_IDF_TARGET_ESP32C3) && \
    CONFIG_IDF_TARGET_ESP32C3
    p.lcd.i2c.sda_pin = 8;
    p.lcd.i2c.scl_pin = 9;
#endif
    return p;
}

}  // namespace interactive_dashboard::system

#ifdef ESP_PLATFORM
BLUSYS_APP_MAIN_DEVICE(interactive_dashboard::system::spec,
                       interactive_dashboard::system::dashboard_device_profile())
#else
BLUSYS_APP_DASHBOARD(interactive_dashboard::system::spec, "Dashboard")
#endif
