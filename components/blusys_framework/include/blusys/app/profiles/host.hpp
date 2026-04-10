#pragma once

// Host platform profile for blusys::app products.
//
// Names the implicit configuration that BLUSYS_APP_MAIN_HOST already
// uses. Useful for Phase 5 scaffold uniformity and for products that
// want to customise the host window size or title.

namespace blusys::app::profiles {

struct host_profile {
    int         hor_res = 480;
    int         ver_res = 320;
    const char *title   = "Blusys App";
};

inline host_profile host_default() { return {}; }

}  // namespace blusys::app::profiles
