#pragma once

// flow_base — lifecycle interface for spec-selectable UI flows.
//
// Flows register their screens and overlays during start(). The framework
// calls start() on each flow before the app's on_init hook fires, so flow
// screens are available when on_init navigates to them.
//
// Usage (in app spec):
//   static blusys::flows::settings_flow kSettings{...};
//   static blusys::flows::flow_base *kFlows[] = {&kSettings};
//   spec.flows      = kFlows;
//   spec.flow_count = 1;

#ifdef BLUSYS_FRAMEWORK_HAS_UI

namespace blusys { class app_ctx; }

namespace blusys::flows {

class flow_base {
public:
    virtual ~flow_base() = default;

    // Called by the framework during app_runtime::init(), before on_init.
    virtual void start(blusys::app_ctx &ctx) = 0;

    // Called during app_runtime::deinit().
    virtual void stop() {}
};

}  // namespace blusys::flows

#endif  // BLUSYS_FRAMEWORK_HAS_UI
