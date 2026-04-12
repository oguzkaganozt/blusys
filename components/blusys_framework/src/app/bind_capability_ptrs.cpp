#include "blusys/app/app_ctx.hpp"
#include "blusys/app/capability_list.hpp"
#include "blusys/app/capabilities/bluetooth.hpp"
#include "blusys/app/capabilities/connectivity.hpp"
#include "blusys/app/capabilities/diagnostics.hpp"
#ifndef ESP_PLATFORM
#include "blusys/app/capabilities/mqtt_host.hpp"
#endif
#include "blusys/app/capabilities/ota.hpp"
#include "blusys/app/capabilities/provisioning.hpp"
#include "blusys/app/capabilities/storage.hpp"
#include "blusys/app/capabilities/telemetry.hpp"

#include <cstddef>

namespace blusys::app {

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
            ctx.provisioning_ = static_cast<provisioning_capability *>(c);
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

}  // namespace blusys::app
