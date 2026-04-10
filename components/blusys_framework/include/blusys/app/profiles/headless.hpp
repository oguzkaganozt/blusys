#pragma once

// Headless platform profile for blusys::app products.
//
// Headless products have no display or input hardware. This struct
// exists for symmetry with device_profile and host_profile, and for
// future extension (e.g. network or storage config for connected
// bundles in Phase 4).

namespace blusys::app::profiles {

struct headless_profile {};

inline headless_profile headless_default() { return {}; }

}  // namespace blusys::app::profiles
