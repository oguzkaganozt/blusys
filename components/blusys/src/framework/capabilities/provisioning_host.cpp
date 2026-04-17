#include "blusys/framework/capabilities/provisioning.hpp"
#include "blusys/framework/engine/intent.hpp"
#include "blusys/framework/engine/event_queue.hpp"
#include "blusys/hal/log.h"

#include <cstring>

static const char *TAG = "blusys_prov";

namespace blusys {

blusys_err_t provisioning_capability::start(blusys::runtime &rt)
{
    rt_ = &rt;
    first_poll_ = true;

    // On host, simulate already-provisioned state.
    status_.is_provisioned = true;
    status_.capability_ready = true;
    std::strncpy(status_.qr_payload,
                 R"({"ver":"v1","name":"HOST_SIM","transport":"ble"})",
                 sizeof(status_.qr_payload) - 1);

    BLUSYS_LOGI(TAG, "host stub: provisioning ready (simulated, already provisioned)");
    return BLUSYS_OK;
}

void provisioning_capability::poll(std::uint32_t /*now_ms*/)
{
    if (!first_poll_) {
        return;
    }
    first_poll_ = false;

    post_event(provisioning_event::already_provisioned);
    post_event(provisioning_event::capability_ready);
}

void provisioning_capability::stop()
{
    status_ = {};
    rt_ = nullptr;
}

blusys_err_t provisioning_capability::request_reset()
{
    BLUSYS_LOGI(TAG, "host stub: provisioning reset (simulated)");
    status_.is_provisioned = false;
    status_.provision_success = false;
    status_.credentials_received = false;
    status_.capability_ready = false;
    post_event(provisioning_event::reset_complete);
    return BLUSYS_OK;
}

}  // namespace blusys

