#pragma once

#include "blusys/framework/app/app.hpp"

#include "core/app_logic.hpp"

namespace mqtt_dashboard::ui {

void on_init(blusys::app::app_ctx &ctx, blusys::app::app_services &svc, app_state &state);

}  // namespace mqtt_dashboard::ui
