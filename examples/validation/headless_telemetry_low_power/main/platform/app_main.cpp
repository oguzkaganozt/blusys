// Validation: headless connectivity + telemetry with CONFIG_PM_ENABLE (low_power policy).

#include "core/app_logic.hpp"

#include "blusys/framework/capabilities/connectivity.hpp"
#include "blusys/framework/capabilities/telemetry.hpp"
#include "blusys/log.h"

namespace telemetry_lp::system {

namespace {

bool deliver_metrics(const blusys::app::telemetry_metric *metrics, std::size_t count,
                     void * /*user_ctx*/)
{
    BLUSYS_LOGI("htlm_lp", "telemetry deliver: %u metrics", static_cast<unsigned>(count));
    for (std::size_t i = 0; i < count; ++i) {
        BLUSYS_LOGI("htlm_lp", "  [%u] %s = %.2f",
                    static_cast<unsigned>(i),
                    metrics[i].name ? metrics[i].name : "?",
                    static_cast<double>(metrics[i].value));
    }
    return true;
}

blusys::app::connectivity_config conn_cfg{
#ifdef ESP_PLATFORM
    .wifi_ssid     = CONFIG_WIFI_SSID,
    .wifi_password = CONFIG_WIFI_PASSWORD,
#else
    .wifi_ssid     = "host-sim",
    .wifi_password = "",
#endif
    .reconnect_delay_ms = 5000,
    .max_retries        = 10,
    .sntp_server        = "pool.ntp.org",
    .prov_service_name  = "htlm-lp",
    .prov_pop           = "lp12345",
    .prov_skip_if_provisioned = true,
};

blusys::app::connectivity_capability connectivity{conn_cfg};

blusys::app::telemetry_capability telemetry{{
    .deliver         = deliver_metrics,
    .flush_threshold = 4,
    .flush_interval_ms = 60000,
}};

blusys::app::capability_list capabilities{&connectivity, &telemetry};

const blusys::app::app_spec<app_state, action> spec{
    .initial_state = {},
    .update        = update,
    .capability_event_discriminant =
        static_cast<std::uint32_t>(action_tag::capability_event),
    .capabilities  = &capabilities,
};

}  // namespace

}  // namespace telemetry_lp::system

BLUSYS_APP_MAIN_HEADLESS(telemetry_lp::system::spec)
