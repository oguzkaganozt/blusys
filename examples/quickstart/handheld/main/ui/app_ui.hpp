#pragma once

#include "core/app_logic.hpp"
#include "blusys/blusys.hpp"

namespace handheld_starter::ui {

void on_init(blusys::app_ctx &ctx, blusys::app_fx &fx, app_state &state);
void update_provisioning_ui(const blusys::connectivity_provisioning_status &status);

extern blusys::flows::settings_flow     kSettingsFlow;
extern blusys::flows::provisioning_flow kProvisioningFlow;

}  // namespace handheld_starter::ui
