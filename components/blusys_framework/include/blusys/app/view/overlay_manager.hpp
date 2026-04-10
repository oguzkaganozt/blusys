#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/core/containers.hpp"
#include "blusys/framework/core/router.hpp"
#include "blusys/framework/ui/widgets/overlay/overlay.hpp"

#include <cstdint>

namespace blusys::app::view {

// Manages overlay widgets and applies route commands (show_overlay / hide_overlay)
// to real LVGL overlay handles. Replaces the Phase 1 logging-only route sink.
//
// Products register overlays by ID during on_init. When the reducer calls
// ctx.show_overlay(id) or ctx.hide_overlay(id), the route command flows
// through the framework runtime into this sink, which calls the actual
// blusys::ui::overlay_show/hide.

class overlay_manager final : public blusys::framework::route_sink {
public:
    static constexpr std::size_t kMaxOverlays = 8;

    bool register_overlay(std::uint32_t id, lv_obj_t *overlay);

    void submit(const blusys::framework::route_command &command) override;

private:
    struct entry {
        std::uint32_t id     = 0;
        lv_obj_t     *widget = nullptr;
    };

    lv_obj_t *find(std::uint32_t id) const;

    blusys::static_vector<entry, kMaxOverlays> overlays_{};
};

// Convenience: create an overlay widget and register it with the manager.
lv_obj_t *overlay_create(lv_obj_t *parent,
                         std::uint32_t route_id,
                         const blusys::ui::overlay_config &config,
                         overlay_manager &mgr);

}  // namespace blusys::app::view

#endif  // BLUSYS_FRAMEWORK_HAS_UI
