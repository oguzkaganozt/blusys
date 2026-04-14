#pragma once

// Umbrella include for the blusys::app product-facing API.
//
// Product code should include this single header. It provides:
//   - app_spec<State, Action>  — define your app
//   - app_identity             — product name, theme, feedback preset
//   - app_ctx                  — dispatch (returns whether queued), capability status, feedback
//   - app_services (via ctx.services()) — routing, overlays, screen_router/shell, ESP FS accessors
//   - entry macros             — BLUSYS_APP, BLUSYS_APP_INTERACTIVE, BLUSYS_APP_DASHBOARD,
//                                  BLUSYS_APP_MAIN_* (escape hatches)
//   - theme presets            — expressive_dark, operational_light, oled
//   - feedback presets         — expressive, operational
//
// For device targets, also include a profile header:
//   #include "blusys/framework/platform/profiles/st7735.hpp"
// Optional layout hints when supporting multiple display sizes:
//   #include "blusys/framework/platform/layout_surface.hpp"
// Optional integration helpers (typed map_event, variant actions):
//   #include 

#include "blusys/framework/app/spec.hpp"
#include "blusys/framework/app/identity.hpp"
#include "blusys/framework/app/ctx.hpp"
#include "blusys/framework/capabilities/list.hpp"
// Capability headers: complete *_status types for app_ctx accessor use via umbrella include.
#include "blusys/framework/capabilities/connectivity.hpp"
#include "blusys/framework/capabilities/storage.hpp"
#include "blusys/framework/capabilities/bluetooth.hpp"
#include "blusys/framework/capabilities/ota.hpp"
#include "blusys/framework/capabilities/diagnostics.hpp"
#include "blusys/framework/capabilities/telemetry.hpp"
#include "blusys/framework/capabilities/lan_control.hpp"
#include "blusys/framework/capabilities/usb.hpp"
#include "blusys/framework/app/entry.hpp"

#ifdef BLUSYS_FRAMEWORK_HAS_UI
#include "blusys/framework/ui/style/presets.hpp"
#include "blusys/framework/ui/composition/view.hpp"
#endif
