#include "blusys/framework/capabilities/lan_control.hpp"
#include "blusys/framework/engine/event_queue.hpp"
#include "blusys/hal/log.h"
#include "blusys/framework/services/net.h"

static const char *TAG = "blusys_lan";

namespace blusys {

struct lan_control_capability::impl {
    blusys_local_ctrl_t *ctrl = nullptr;
    blusys_mdns_t       *mdns = nullptr;
};

lan_control_capability::lan_control_capability(const lan_control_config &cfg)
    : cfg_(cfg), impl_(new impl{})
{
}

lan_control_capability::~lan_control_capability()
{
    delete impl_;
}

blusys_err_t lan_control_capability::start(blusys::runtime &rt)
{
    rt_ = &rt;

    if (cfg_.device_name == nullptr) {
        BLUSYS_LOGE(TAG, "device_name is required");
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_local_ctrl_config_t ctrl_cfg{};
    ctrl_cfg.device_name      = cfg_.device_name;
    ctrl_cfg.http_port        = cfg_.http_port;
    ctrl_cfg.actions          = cfg_.actions;
    ctrl_cfg.action_count     = cfg_.action_count;
    ctrl_cfg.max_body_len     = cfg_.max_body_len;
    ctrl_cfg.max_response_len = cfg_.max_response_len;
    ctrl_cfg.status_cb        = cfg_.status_cb;
    ctrl_cfg.user_ctx         = cfg_.user_ctx;

    blusys_err_t err = blusys_local_ctrl_open(&ctrl_cfg, &impl_->ctrl);
    if (err != BLUSYS_OK) {
        BLUSYS_LOGE(TAG, "local_ctrl open failed: %d", static_cast<int>(err));
        return err;
    }

    status_.http_listening = true;
    post_event(lan_control_event::http_ready);

    if (cfg_.advertise_mdns) {
        const char *hostname = cfg_.service_name != nullptr ? cfg_.service_name : cfg_.device_name;
        const char *instance = cfg_.instance_name != nullptr ? cfg_.instance_name : cfg_.device_name;

        blusys_mdns_config_t mdns_cfg{};
        mdns_cfg.hostname = hostname;
        mdns_cfg.instance_name = instance;

        err = blusys_mdns_open(&mdns_cfg, &impl_->mdns);
        if (err != BLUSYS_OK) {
            BLUSYS_LOGE(TAG, "mdns open failed: %d", static_cast<int>(err));
            stop();
            return err;
        }

        err = blusys_mdns_add_service(impl_->mdns, instance,
                                      cfg_.service_type != nullptr ? cfg_.service_type : "_http",
                                      static_cast<blusys_mdns_proto_t>(cfg_.service_proto),
                                      cfg_.http_port,
                                      nullptr,
                                      0);
        if (err != BLUSYS_OK) {
            BLUSYS_LOGE(TAG, "mdns service add failed: %d", static_cast<int>(err));
            stop();
            return err;
        }

        status_.mdns_announced = true;
        post_event(lan_control_event::mdns_ready);
    }

    check_capability_ready();
    return BLUSYS_OK;
}

void lan_control_capability::poll(std::uint32_t /*now_ms*/)
{
    // Local control and mDNS setup are synchronous.
}

void lan_control_capability::stop()
{
    if (impl_->mdns != nullptr) {
        if (cfg_.advertise_mdns) {
            (void)blusys_mdns_remove_service(
                impl_->mdns,
                cfg_.service_type != nullptr ? cfg_.service_type : "_http",
                static_cast<blusys_mdns_proto_t>(cfg_.service_proto));
        }
        blusys_mdns_close(impl_->mdns);
        impl_->mdns = nullptr;
    }
    if (impl_->ctrl != nullptr) {
        blusys_local_ctrl_close(impl_->ctrl);
        impl_->ctrl = nullptr;
    }

    status_ = {};
    rt_ = nullptr;
}

void lan_control_capability::check_capability_ready()
{
    if (status_.capability_ready) {
        return;
    }

    bool ready = status_.http_listening;
    if (cfg_.advertise_mdns && !status_.mdns_announced) {
        ready = false;
    }

    if (ready) {
        status_.capability_ready = true;
        post_event(lan_control_event::capability_ready);
        BLUSYS_LOGI(TAG, "lan control capability ready");
    }
}

}  // namespace blusys
