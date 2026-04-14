#pragma once

// SDL host window sizing for BLUSYS_APP_MAIN_HOST_PROFILE and auto_host_profile_* helpers.

#include <cstdint>

namespace blusys {

/// Passed to BLUSYS_APP_MAIN_HOST_PROFILE to control the SDL2 window.
struct host_profile {
    int         hor_res = 480;
    int         ver_res = 320;
    const char *title   = "Blusys App";
};

inline host_profile host_default() { return {}; }

}  // namespace blusys
