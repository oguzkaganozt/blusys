#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/view/screen_registry.hpp"
#include "blusys/app/app_ctx.hpp"
#include "blusys/log.h"

static const char *TAG = "screen_reg";

namespace blusys::app::view {

bool screen_registry::register_screen(std::uint32_t    route_id,
                                       screen_create_fn  create_fn,
                                       screen_destroy_fn destroy_fn)
{
    if (create_fn == nullptr) {
        return false;
    }
    if (entries_.size() >= kMaxScreens) {
        BLUSYS_LOGW(TAG, "screen registry full (max %zu)", kMaxScreens);
        return false;
    }
    entries_.push_back({
        .route_id   = route_id,
        .create_fn  = create_fn,
        .destroy_fn = destroy_fn,
    });
    return true;
}

bool screen_registry::navigate(const blusys::framework::route_command &cmd, app_ctx &ctx)
{
    using T = blusys::framework::route_command_type;
    switch (cmd.type) {
    case T::set_root:
        nav_stack_.clear();
        nav_stack_.push_back(cmd.id);
        load_screen(cmd.id, cmd.params, ctx);
        return true;

    case T::push:
        if (nav_stack_.size() < kMaxStackDepth) {
            nav_stack_.push_back(cmd.id);
            load_screen(cmd.id, cmd.params, ctx);
            return true;
        }
        BLUSYS_LOGW(TAG, "nav stack full, push ignored");
        return false;

    case T::replace:
        if (!nav_stack_.empty()) {
            nav_stack_[nav_stack_.size() - 1] = cmd.id;
        } else {
            nav_stack_.push_back(cmd.id);
        }
        load_screen(cmd.id, cmd.params, ctx);
        return true;

    case T::pop:
        if (nav_stack_.size() > 1) {
            nav_stack_.pop_back();
            const std::uint32_t prev = nav_stack_[nav_stack_.size() - 1];
            load_screen(prev, nullptr, ctx);
            return true;
        }
        BLUSYS_LOGW(TAG, "nav stack at root, pop ignored");
        return false;

    default:
        return false;
    }
}

void screen_registry::set_screen_changed_callback(void (*fn)(lv_obj_t *, void *),
                                                    void *user_ctx)
{
    on_screen_changed_ = fn;
    screen_changed_ctx_ = user_ctx;
}

const screen_registry::screen_entry *screen_registry::find(std::uint32_t id) const
{
    for (const auto &e : entries_) {
        if (e.route_id == id) {
            return &e;
        }
    }
    return nullptr;
}

void screen_registry::load_screen(std::uint32_t route_id,
                                   const void   *params,
                                   app_ctx      &ctx)
{
    const screen_entry *entry = find(route_id);
    if (entry == nullptr) {
        BLUSYS_LOGW(TAG, "no screen registered for route %lu",
                    static_cast<unsigned long>(route_id));
        return;
    }

    destroy_active();

    lv_group_t *group  = nullptr;
    lv_obj_t   *screen = entry->create_fn(ctx, params, &group);
    if (screen == nullptr) {
        BLUSYS_LOGW(TAG, "create_fn returned null for route %lu",
                    static_cast<unsigned long>(route_id));
        return;
    }

    active_screen_     = screen;
    active_destroy_fn_ = entry->destroy_fn;

    lv_screen_load(screen);

    // Attach focus group to all registered encoder indevs.
    // Covers the host keyboard encoder (and any device encoder registered
    // before screen_router wires its own callback below).
    if (group != nullptr) {
        lv_indev_t *indev = lv_indev_get_next(nullptr);
        while (indev != nullptr) {
            if (lv_indev_get_type(indev) == LV_INDEV_TYPE_ENCODER) {
                lv_indev_set_group(indev, group);
            }
            indev = lv_indev_get_next(indev);
        }
    }

    if (on_screen_changed_ != nullptr) {
        on_screen_changed_(screen, screen_changed_ctx_);
    }
}

void screen_registry::destroy_active()
{
    if (active_screen_ == nullptr) {
        return;
    }
    if (active_destroy_fn_ != nullptr) {
        active_destroy_fn_(active_screen_);
    } else {
        lv_obj_delete(active_screen_);
    }
    active_screen_     = nullptr;
    active_destroy_fn_ = nullptr;
}

}  // namespace blusys::app::view

#endif  // BLUSYS_FRAMEWORK_HAS_UI
