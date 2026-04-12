#pragma once

#include "blusys/framework/core/containers.hpp"
#include "blusys/framework/core/feedback.hpp"
#include "blusys/framework/core/intent.hpp"
#include "blusys/framework/core/router.hpp"
#include "blusys/framework/core/runtime.hpp"

namespace blusys::framework {

// Umbrella header for the framework spine — aggregating includes only.
// No global init is required: `blusys::app` entry macros and validation
// examples instantiate `blusys::framework::runtime` directly, and the
// widget kit has no global state beyond the theme (set via
// `blusys::ui::set_theme`). The former `init()` / `run_once()` no-ops
// were removed because they misdirected readers looking for the real
// entry path (`BLUSYS_APP_MAIN_*`).

}  // namespace blusys::framework
