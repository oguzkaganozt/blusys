#include "blusys/framework/capabilities/usb.hpp"
#include "blusys/framework/events/event_queue.hpp"
#include "blusys/hal/log.h"

static const char *TAG = "blusys_usb_cap";

namespace blusys {

struct usb_capability::impl {
    bool first_poll = true;
};

usb_capability::usb_capability(const usb_config &cfg)
    : impl_(new impl{}), cfg_(cfg)
{
}

usb_capability::~usb_capability()
{
    delete impl_;
}

blusys_err_t usb_capability::start(blusys::runtime &rt)
{
    rt_ = &rt;
    impl_->first_poll = true;
    status_.role_active = false;
    status_.capability_ready = false;

    BLUSYS_LOGI(TAG, "host stub: usb not supported on SDL host");
    return BLUSYS_ERR_NOT_SUPPORTED;
}

void usb_capability::poll(std::uint32_t /*now_ms*/)
{
    if (!impl_->first_poll) {
        return;
    }
    impl_->first_poll = false;
}

void usb_capability::stop()
{
    status_ = {};
    rt_ = nullptr;
}

void usb_capability::device_event_handler(blusys_usb_device_t * /*dev*/, int /*event*/,
                                          void * /*user_ctx*/)
{
}

void usb_capability::host_event_handler(blusys_usb_host_t * /*host*/, int /*event*/,
                                        const void * /*device_info*/, void * /*user_ctx*/)
{
}

void usb_capability::check_capability_ready()
{
}

}  // namespace blusys
