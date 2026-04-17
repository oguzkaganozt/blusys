#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/ui/composition/overlay_manager.hpp"
#include "blusys/hal/log.h"

namespace blusys {

namespace {
constexpr const char *kTag = "blusys_app";
}

bool overlay_manager::register_overlay(std::uint32_t id, lv_obj_t *overlay)
{
    if (overlay == nullptr) {
        return false;
    }
    return overlays_.push_back({.id = id, .widget = overlay});
}

void overlay_manager::submit(const blusys::route_command &command)
{
    using T = blusys::route_command_type;
    const bool is_show = (command.type == T::show_overlay);
    if (!is_show && command.type != T::hide_overlay) {
        return;
    }

    lv_obj_t *w = find(command.id);
    if (w == nullptr) {
        BLUSYS_LOGW(kTag, "%s: unknown id=%lu",
                    is_show ? "show_overlay" : "hide_overlay",
                    static_cast<unsigned long>(command.id));
        return;
    }

    if (is_show) {
        blusys::overlay_show(w);
        if (focus_scope_ != nullptr) {
            focus_scope_->push_scope(w);
        }
    } else {
        blusys::overlay_hide(w);
        if (focus_scope_ != nullptr) {
            if (!focus_scope_->remove_scope(w)) {
                BLUSYS_LOGW(kTag, "hide_overlay: scope not found for id=%lu",
                            static_cast<unsigned long>(command.id));
            }
        }
    }
}

lv_obj_t *overlay_manager::find(std::uint32_t id) const
{
    for (std::size_t i = 0; i < overlays_.size(); ++i) {
        if (overlays_[i].id == id) {
            return overlays_[i].widget;
        }
    }
    return nullptr;
}

lv_obj_t *overlay_create(lv_obj_t *parent,
                         std::uint32_t route_id,
                         const blusys::overlay_config &config,
                         overlay_manager &mgr)
{
    lv_obj_t *overlay = blusys::overlay_create(parent, config);
    if (overlay != nullptr) {
        mgr.register_overlay(route_id, overlay);
    }
    return overlay;
}

}  // namespace blusys

#endif  // BLUSYS_FRAMEWORK_HAS_UI
