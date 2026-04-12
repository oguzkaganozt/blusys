#pragma once

// Umbrella include for the blusys::app product-facing API.
//
// Product code should include this single header. It provides:
//   - app_spec<State, Action>  — define your app
//   - app_identity             — product name, theme, feedback preset
//   - app_ctx                  — dispatch (returns whether queued), navigate, feedback
//   - entry macros             — BLUSYS_APP, BLUSYS_APP_INTERACTIVE, BLUSYS_APP_DASHBOARD,
//                                  BLUSYS_APP_MAIN_* (escape hatches)
//   - theme presets            — expressive_dark, operational_light, oled
//   - feedback presets         — expressive, operational
//
// For device targets, also include a profile header:
//   #include "blusys/app/profiles/st7735.hpp"
// Optional layout hints when supporting multiple display sizes:
//   #include "blusys/app/layout_surface.hpp"
// Optional integration helpers (typed map_event, variant actions):
//   #include "blusys/app/integration.hpp"

#include "blusys/app/app_spec.hpp"
#include "blusys/app/app_identity.hpp"
#include "blusys/app/app_ctx.hpp"
#include "blusys/app/capability_list.hpp"
// Capability headers: complete *_status types for app_ctx accessor use via umbrella include.
#include "blusys/app/capabilities/connectivity.hpp"
#include "blusys/app/capabilities/storage.hpp"
#include "blusys/app/capabilities/bluetooth.hpp"
#include "blusys/app/capabilities/ota.hpp"
#include "blusys/app/capabilities/diagnostics.hpp"
#include "blusys/app/capabilities/telemetry.hpp"
#include "blusys/app/capabilities/provisioning.hpp"
#include "blusys/app/entry.hpp"

#ifdef BLUSYS_FRAMEWORK_HAS_UI
#include "blusys/app/theme_presets.hpp"
#include "blusys/app/view/view.hpp"
#endif
