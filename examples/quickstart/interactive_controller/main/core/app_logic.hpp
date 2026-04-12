#pragma once

#include <cstdint>

#include "blusys/app/app.hpp"
#include "blusys/app/capability_event.hpp"
#include "blusys/app/capabilities/provisioning.hpp"
#include "blusys/app/flows/provisioning_flow.hpp"

#include "ui/panels.hpp"

namespace interactive_controller {

enum class route : std::uint32_t {
    home = 1,
    status,
    settings,
    setup,
    about,
};

enum class overlay : std::uint32_t {
    confirm = 1,
};

enum class action_tag : std::uint8_t {
    capability_event,
    level_delta,
    toggle_hold,
    set_accent,
    confirm,
    show_home,
    show_status,
    show_settings,
    open_setup,
    open_about,
};

struct action {
    action_tag                      tag = action_tag::show_home;
    std::int32_t                    value = 0;
    blusys::app::capability_event   cap_event{};
};

struct app_state {
    std::int32_t level        = 64;
    bool         hold_enabled = false;
    std::int32_t accent_index = 1;
    bool         storage_ready = false;
    blusys::app::provisioning_status provisioning{};

    ui::ShellChrome shell{};
    ui::HomePanel   home{};
    ui::StatusPanel status{};

    blusys::app::flows::provisioning_screen_handles setup_handles{};
};

void update(blusys::app::app_ctx &ctx, app_state &state, const action &event);
bool map_intent(blusys::app::app_services &svc, blusys::framework::intent intent, action *out);

const char *accent_name(std::int32_t index);

}  // namespace interactive_controller
