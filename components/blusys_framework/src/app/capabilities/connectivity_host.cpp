#ifndef ESP_PLATFORM

#include "blusys/app/capabilities/connectivity.hpp"
#include "blusys/framework/core/intent.hpp"
#include "blusys/framework/core/runtime.hpp"
#include "blusys/log.h"

static const char *TAG = "blusys_conn";

namespace blusys::app {

blusys_err_t connectivity_capability::start(blusys::framework::runtime &rt)
{
    rt_ = &rt;
    first_poll_ = true;

    // On host, immediately mark everything as connected.
    status_.wifi_connected = true;
    status_.has_ip         = true;

    if (cfg_.sntp_server != nullptr) {
        status_.time_synced = true;
    }
    if (cfg_.mdns_hostname != nullptr) {
        status_.mdns_running = true;
    }
    if (cfg_.local_ctrl_device_name != nullptr) {
        status_.local_ctrl_running = true;
    }

    status_.capability_ready = true;

    BLUSYS_LOGI(TAG, "host stub: connectivity ready (simulated)");
    return BLUSYS_OK;
}

void connectivity_capability::poll(std::uint32_t /*now_ms*/)
{
    if (!first_poll_) {
        return;
    }
    first_poll_ = false;

    // Post simulated events on the first poll so the app reducer
    // receives them through the normal event flow (not during init).
    status_.wifi_connecting = true;
    post_event(connectivity_event::wifi_started);
    post_event(connectivity_event::wifi_connecting);
    status_.wifi_connected = true;
    status_.wifi_connecting = false;
    status_.wifi_reconnecting = false;
    post_event(connectivity_event::wifi_connected);
    post_event(connectivity_event::wifi_got_ip);

    if (cfg_.sntp_server != nullptr) {
        post_event(connectivity_event::time_synced);
    }
    if (cfg_.mdns_hostname != nullptr) {
        post_event(connectivity_event::mdns_ready);
    }
    if (cfg_.local_ctrl_device_name != nullptr) {
        post_event(connectivity_event::local_ctrl_ready);
    }

    post_event(connectivity_event::capability_ready);
}

void connectivity_capability::stop()
{
    status_ = {};
    rt_ = nullptr;
}

blusys_err_t connectivity_capability::request_reconnect()
{
    BLUSYS_LOGI(TAG, "host stub: reconnect requested (simulated)");
    status_.wifi_connecting = true;
    status_.wifi_reconnecting = false;
    post_event(connectivity_event::wifi_connecting);
    status_.wifi_connected = true;
    status_.wifi_connecting = false;
    post_event(connectivity_event::wifi_connected);
    post_event(connectivity_event::wifi_got_ip);
    return BLUSYS_OK;
}

void connectivity_capability::post_event(connectivity_event ev)
{
    if (rt_ != nullptr) {
        rt_->post_event(blusys::framework::make_integration_event(
            static_cast<std::uint32_t>(ev)));
    }
}

}  // namespace blusys::app

#endif  // !ESP_PLATFORM
