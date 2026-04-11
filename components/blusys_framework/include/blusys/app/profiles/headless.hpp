#pragma once

// Headless platform profile for blusys::app products.
//
// Headless products have no display or input hardware. Use with
// `BLUSYS_APP_MAIN_HEADLESS_PROFILE` for the same code-first configuration
// style as `host_profile` / `device_profile`. Runtime cadence and
// capabilities remain on `app_spec` and `integration/`.

namespace blusys::app::profiles {

struct headless_profile {
    /// Optional label for the first boot log line after runtime init (nullptr to skip).
    const char *boot_log_label = nullptr;
};

inline headless_profile headless_default()
{
    return {};
}

}  // namespace blusys::app::profiles
