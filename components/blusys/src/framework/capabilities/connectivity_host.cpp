#include "blusys/framework/capabilities/connectivity.hpp"
#include "blusys/framework/events/event.hpp"
#include "blusys/framework/events/event_queue.hpp"
#include "blusys/hal/log.h"

#include <cstring>

static const char *TAG = "blusys_conn";

namespace blusys {

struct connectivity_capability::impl {};

connectivity_capability::connectivity_capability(const connectivity_config &cfg)
    : impl_(new impl{}), cfg_(cfg)
{
}

connectivity_capability::~connectivity_capability()
{
    delete impl_;
}

blusys_err_t connectivity_capability::start(blusys::runtime &rt)
{
    rt_ = &rt;
    first_poll_ = true;

    if (cfg_.wifi_ssid != nullptr) {
        status_.wifi_connected = true;
        status_.has_ip         = true;
    }
    if (cfg_.sntp_server != nullptr) {
        status_.time_synced = true;
    }
    if (cfg_.mdns_hostname != nullptr) {
        status_.mdns_running = true;
    }
    if (cfg_.local_ctrl_device_name != nullptr) {
        status_.local_ctrl_running = true;
    }
    if (cfg_.prov_service_name != nullptr) {
        status_.provisioning.is_provisioned = true;
        status_.provisioning.capability_ready = true;
        std::strncpy(status_.provisioning.qr_payload,
                     R"({"ver":"v1","name":"HOST_SIM","transport":"ble"})",
                     sizeof(status_.provisioning.qr_payload) - 1);
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

    if (cfg_.wifi_ssid != nullptr) {
        status_.wifi_connecting = true;
        post_event(connectivity_event::wifi_started);
        post_event(connectivity_event::wifi_connecting);
        status_.wifi_connected = true;
        status_.wifi_connecting = false;
        status_.wifi_reconnecting = false;
        post_event(connectivity_event::wifi_connected);
        post_event(connectivity_event::wifi_got_ip);
    }
    if (cfg_.sntp_server != nullptr) {
        post_event(connectivity_event::time_synced);
    }
    if (cfg_.mdns_hostname != nullptr) {
        post_event(connectivity_event::mdns_ready);
    }
    if (cfg_.local_ctrl_device_name != nullptr) {
        post_event(connectivity_event::local_ctrl_ready);
    }
    if (cfg_.prov_service_name != nullptr) {
        post_event(connectivity_event::prov_already_done);
        post_event(connectivity_event::provisioning_ready);
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

void connectivity_capability::wifi_event_handler(blusys_wifi_t * /*wifi*/, int /*event*/,
                                                  const void * /*info*/, void * /*user_ctx*/)
{
}

void connectivity_capability::prov_event_handler(int /*event*/, const void * /*creds*/,
                                                  void * /*user_ctx*/)
{
}

void connectivity_capability::start_dependent_services(std::uint32_t /*now_ms*/)
{
}

void connectivity_capability::check_capability_ready()
{
}

}  // namespace blusys
