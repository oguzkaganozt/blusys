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
    switch (command.type) {
    case blusys::route_command_type::show_overlay: {
        lv_obj_t *w = find(command.id);
        if (w != nullptr) {
            blusys::overlay_show(w);
            if (focus_scope_ != nullptr) {
                focus_scope_->push_scope(w);
            }
        } else {
            BLUSYS_LOGW(kTag, "show_overlay: unknown id=%lu",
                        static_cast<unsigned long>(command.id));
        }
        break;
    }
    case blusys::route_command_type::hide_overlay: {
        lv_obj_t *w = find(command.id);
        if (w != nullptr) {
            blusys::overlay_hide(w);
            if (focus_scope_ != nullptr) {
                focus_scope_->pop_scope();
            }
        } else {
            BLUSYS_LOGW(kTag, "hide_overlay: unknown id=%lu",
                        static_cast<unsigned long>(command.id));
        }
        break;
    }
    default:
        BLUSYS_LOGI(kTag, "route: %s id=%lu",
                    blusys::route_command_type_name(command.type),
                    static_cast<unsigned long>(command.id));
        break;
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
