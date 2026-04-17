// Connected headless — capabilities, app_spec, entry (see core/ for reducer).

#include "core/app_logic.hpp"
#include "blusys/blusys.hpp"


#include <cstdio>

namespace {

blusys_err_t status_cb(char *json_buf, size_t buf_len,
                       size_t *out_len, void * /*user_ctx*/)
{
    int written = std::snprintf(json_buf, buf_len,
                                "{\"product\":\"connected-headless-demo\"}");
    if (written < 0 || static_cast<size_t>(written) >= buf_len) {
        return BLUSYS_ERR_NO_MEM;
    }
    *out_len = static_cast<size_t>(written);
    return BLUSYS_OK;
}

static blusys::connectivity_capability conn{{
    .wifi_ssid     = CONFIG_WIFI_SSID,
    .wifi_password = CONFIG_WIFI_PASSWORD,

    .sntp_server = "pool.ntp.org",

    .mdns_hostname      = "blusys-headless",
    .mdns_instance_name = "Blusys Headless Demo",

    .local_ctrl_device_name = "Blusys Headless",
    .local_ctrl_status_cb   = status_cb,
}};

static blusys::storage_capability stor{{
    .spiffs_base_path = "/fs",
}};

static blusys::capability_list_storage capabilities{&conn, &stor};

static const blusys::app_spec<connected_headless::State, connected_headless::Action> spec{
    .initial_state  = {},
    .update         = connected_headless::update,
    .on_tick        = connected_headless::on_tick,
    .on_event       = connected_headless::on_event,
    .tick_period_ms = 100,
    .capabilities   = &capabilities,
};

}  // namespace

BLUSYS_APP_HEADLESS(spec)
