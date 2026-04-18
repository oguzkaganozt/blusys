#include "blusys/framework/capabilities/lan_control.hpp"
#include "blusys/framework/engine/event_queue.hpp"
#include "blusys/hal/log.h"

static const char *TAG = "blusys_lan";

namespace blusys {

struct lan_control_capability::impl {
    bool first_poll = true;
};

lan_control_capability::lan_control_capability(const lan_control_config &cfg)
    : impl_(new impl{}), cfg_(cfg)
{
}

lan_control_capability::~lan_control_capability()
{
    delete impl_;
}

blusys_err_t lan_control_capability::start(blusys::runtime &rt)
{
    rt_ = &rt;
    impl_->first_poll = true;

    if (cfg_.device_name == nullptr) {
        BLUSYS_LOGE(TAG, "device_name is required");
        return BLUSYS_ERR_INVALID_ARG;
    }

    status_.http_listening = true;
    status_.mdns_announced = cfg_.advertise_mdns;
    status_.capability_ready = true;

    BLUSYS_LOGI(TAG, "host stub: lan control ready (simulated)");
    return BLUSYS_OK;
}

void lan_control_capability::poll(std::uint32_t /*now_ms*/)
{
    if (!impl_->first_poll) {
        return;
    }
    impl_->first_poll = false;

    post_event(lan_control_event::http_ready);
    if (cfg_.advertise_mdns) {
        post_event(lan_control_event::mdns_ready);
    }
    post_event(lan_control_event::capability_ready);
}

void lan_control_capability::stop()
{
    status_ = {};
    rt_ = nullptr;
}

void lan_control_capability::check_capability_ready()
{
}

}  // namespace blusys
