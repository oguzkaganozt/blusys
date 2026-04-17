#pragma once

#include <cstdint>
#include <optional>
#include "blusys/blusys.hpp"


#include "ui/panels.hpp"

namespace handheld_starter {

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
    blusys::capability_event   cap_event{};
};

struct app_state {
    std::int32_t level        = 64;
    bool         hold_enabled = false;
    std::int32_t accent_index = 1;
    bool         storage_ready = false;
    blusys::connectivity_provisioning_status provisioning{};

    ui::ShellChrome shell{};
    ui::HomePanel   home{};
    ui::StatusPanel status{};
};

void update(blusys::app_ctx &ctx, app_state &state, const action &event);
std::optional<action> on_event(blusys::event event, app_state &state);

const char *accent_name(std::int32_t index);

}  // namespace handheld_starter
