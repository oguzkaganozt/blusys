#include "core/app_logic.hpp"

#include "blusys/framework/capabilities/usb.hpp"

namespace usb_basic::system {

namespace {

blusys::app::usb_capability usb{{
    .role = blusys::app::usb_role::host,
    .class_mask = static_cast<std::uint8_t>(blusys::app::usb_class::cdc),
    .manufacturer = "Blusys",
    .product = "USB Basic",
    .serial = "usb-basic-01",
}};

blusys::app::capability_list capabilities{&usb};

const blusys::app::app_spec<app_state, action> spec{
    .initial_state = {},
    .update = update,
    .capability_event_discriminant =
        static_cast<std::uint32_t>(action_tag::capability_event),
    .capabilities = &capabilities,
};

}  // namespace

}  // namespace usb_basic::system

BLUSYS_APP_MAIN_HEADLESS(usb_basic::system::spec)
