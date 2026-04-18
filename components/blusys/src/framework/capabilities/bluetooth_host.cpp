#include "blusys/framework/capabilities/bluetooth.hpp"
#include "blusys/framework/engine/intent.hpp"
#include "blusys/framework/engine/event_queue.hpp"
#include "blusys/hal/log.h"

static const char *TAG = "blusys_bt";

namespace blusys {

struct bluetooth_capability::impl {
    bool first_poll = true;
};

bluetooth_capability::bluetooth_capability(const bluetooth_config &cfg)
    : impl_(new impl{}), cfg_(cfg)
{
}

bluetooth_capability::~bluetooth_capability()
{
    delete impl_;
}

blusys_err_t bluetooth_capability::start(blusys::runtime &rt)
{
    rt_ = &rt;
    impl_->first_poll = true;

    status_.gap_ready = true;
    status_.gatt_ready = true;
    status_.advertising = true;

    BLUSYS_LOGI(TAG, "host stub: bluetooth ready (simulated)");
    return BLUSYS_OK;
}

void bluetooth_capability::poll(std::uint32_t /*now_ms*/)
{
    if (!impl_->first_poll) {
        return;
    }
    impl_->first_poll = false;

    status_.capability_ready = true;
    post_event(bluetooth_event::gap_ready);
    post_event(bluetooth_event::gatt_ready);
    post_event(bluetooth_event::advertising_started);
    post_event(bluetooth_event::capability_ready);
}

void bluetooth_capability::stop()
{
    status_ = {};
    rt_ = nullptr;
}

void bluetooth_capability::gatt_conn_handler(std::uint16_t /*conn_handle*/,
                                             bool /*connected*/,
                                             void * /*user_ctx*/)
{
}

void bluetooth_capability::check_capability_ready()
{
}

}  // namespace blusys
