#include "blusys/framework/app/ctx.hpp"
#include "blusys/framework/app/services.hpp"
#include "blusys/framework/capabilities/connectivity.hpp"
#include "blusys/framework/capabilities/storage.hpp"
#include "blusys/framework/capabilities/bluetooth.hpp"
#include "blusys/framework/capabilities/ota.hpp"
#include "blusys/framework/capabilities/diagnostics.hpp"
#include "blusys/framework/capabilities/telemetry.hpp"
#include "blusys/framework/capabilities/lan_control.hpp"
#include "blusys/framework/capabilities/usb.hpp"
#include "blusys/framework/capabilities/list.hpp"
#ifndef ESP_PLATFORM
#include "blusys/framework/capabilities/mqtt_host.hpp"
#endif

namespace blusys {

void app_ctx::emit_feedback(blusys::feedback_channel channel,
                            blusys::feedback_pattern pattern,
                            std::uint32_t value)
{
    if (feedback_bus_ != nullptr) {
        feedback_bus_->emit({
            .channel = channel,
            .pattern = pattern,
            .value   = value,
            .payload = nullptr,
        });
    }
}

const connectivity_status *app_ctx::connectivity() const
{
    if (connectivity_ != nullptr) {
        return &connectivity_->status();
    }
    return nullptr;
}

const storage_status *app_ctx::storage() const
{
    if (storage_ != nullptr) {
        return &storage_->status();
    }
    return nullptr;
}

const bluetooth_status *app_ctx::bluetooth() const
{
    if (bluetooth_ != nullptr) {
        return &bluetooth_->status();
    }
    return nullptr;
}

const ota_status *app_ctx::ota() const
{
    if (ota_ != nullptr) {
        return &ota_->status();
    }
    return nullptr;
}

const diagnostics_status *app_ctx::diagnostics() const
{
    if (diagnostics_ != nullptr) {
        return &diagnostics_->status();
    }
    return nullptr;
}

const telemetry_status *app_ctx::telemetry() const
{
    if (telemetry_ != nullptr) {
        return &telemetry_->status();
    }
    return nullptr;
}

const lan_control_status *app_ctx::lan_control() const
{
    if (lan_control_ != nullptr) {
        return &lan_control_->status();
    }
    return nullptr;
}

const usb_status *app_ctx::usb() const
{
    if (usb_ != nullptr) {
        return &usb_->status();
    }
    return nullptr;
}

mqtt_host_capability *app_ctx::mqtt_host()
{
    return mqtt_host_;
}

blusys_err_t app_ctx::request_connectivity_reconnect()
{
    if (connectivity_ == nullptr) {
        return BLUSYS_ERR_INVALID_STATE;
    }
    return connectivity_->request_reconnect();
}

void app_ctx::bind_capability_pointers_from_list(app_ctx &ctx, capability_list *capabilities)
{
    if (capabilities == nullptr) {
        return;
    }
    for (std::size_t i = 0; i < capabilities->count; ++i) {
        capability_base *c = capabilities->items[i];
        if (c == nullptr) {
            continue;
        }
        switch (c->kind()) {
        case capability_kind::connectivity:
            ctx.connectivity_ = static_cast<connectivity_capability *>(c);
            break;
        case capability_kind::storage:
            ctx.storage_ = static_cast<storage_capability *>(c);
            break;
        case capability_kind::bluetooth:
            ctx.bluetooth_ = static_cast<bluetooth_capability *>(c);
            break;
        case capability_kind::ota:
            ctx.ota_ = static_cast<ota_capability *>(c);
            break;
        case capability_kind::diagnostics:
            ctx.diagnostics_ = static_cast<diagnostics_capability *>(c);
            break;
        case capability_kind::telemetry:
            ctx.telemetry_ = static_cast<telemetry_capability *>(c);
            break;
        case capability_kind::provisioning:
            break;
        case capability_kind::lan_control:
            ctx.lan_control_ = static_cast<lan_control_capability *>(c);
            break;
        case capability_kind::usb:
            ctx.usb_ = static_cast<usb_capability *>(c);
            break;
        case capability_kind::mqtt_host:
#ifndef ESP_PLATFORM
            ctx.mqtt_host_ = static_cast<mqtt_host_capability *>(c);
#endif
            break;
        case capability_kind::custom:
            break;
        }
    }
}

}  // namespace blusys
