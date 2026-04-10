#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/view/screen_registry.hpp"
#include "blusys/app/app_ctx.hpp"
#include "blusys/framework/ui/input/focus_scope.hpp"
#include "blusys/framework/ui/transition.hpp"
#include "blusys/log.h"

static const char *TAG = "screen_reg";

namespace blusys::app::view {

bool screen_registry::register_screen(std::uint32_t    route_id,
                                       screen_create_fn  create_fn,
                                       screen_destroy_fn destroy_fn)
{
    return register_screen(route_id, create_fn, destroy_fn, {});
}

bool screen_registry::register_screen(std::uint32_t         route_id,
                                       screen_create_fn       create_fn,
                                       screen_destroy_fn      destroy_fn,
                                       const screen_lifecycle &lifecycle)
{
    if (create_fn == nullptr) {
        return false;
    }
    if (entries_.size() >= kMaxScreens) {
        BLUSYS_LOGW(TAG, "screen registry full (max %zu)", kMaxScreens);
        return false;
    }
    (void)entries_.push_back({
        .route_id   = route_id,
        .create_fn  = create_fn,
        .destroy_fn = destroy_fn,
        .lifecycle  = lifecycle,
    });
    return true;
}

bool screen_registry::navigate(const blusys::framework::route_command &cmd, app_ctx &ctx)
{
    using T = blusys::framework::route_command_type;
    switch (cmd.type) {
    case T::set_root:
        nav_stack_.clear();
        (void)nav_stack_.push_back({.route_id = cmd.id, .params = cmd.params});
        load_screen(cmd.id, cmd.params, ctx, cmd.transition,
                    blusys::ui::transition_type::none);
        return true;

    case T::push:
        if (nav_stack_.size() < kMaxStackDepth) {
            (void)nav_stack_.push_back({.route_id = cmd.id, .params = cmd.params});
            load_screen(cmd.id, cmd.params, ctx, cmd.transition,
                        blusys::ui::transition_type::slide_left);
            return true;
        }
        BLUSYS_LOGW(TAG, "nav stack full, push ignored");
        return false;

    case T::replace:
        if (!nav_stack_.empty()) {
            nav_stack_[nav_stack_.size() - 1] = {.route_id = cmd.id, .params = cmd.params};
        } else {
            (void)nav_stack_.push_back({.route_id = cmd.id, .params = cmd.params});
        }
        load_screen(cmd.id, cmd.params, ctx, cmd.transition,
                    blusys::ui::transition_type::none);
        return true;

    case T::pop:
        if (nav_stack_.size() > 1) {
            nav_stack_.pop_back();
            const auto &prev = nav_stack_[nav_stack_.size() - 1];
            load_screen(prev.route_id, prev.params, ctx, cmd.transition,
                        blusys::ui::transition_type::slide_right);
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
                                   app_ctx      &ctx,
                                   std::uint32_t transition_field,
                                   blusys::ui::transition_type default_transition)
{
    const screen_entry *entry = find(route_id);
    if (entry == nullptr) {
        BLUSYS_LOGW(TAG, "no screen registered for route %lu",
                    static_cast<unsigned long>(route_id));
        return;
    }

    // Fire lifecycle hooks on the outgoing screen.
    fire_hide_hooks();

    const bool in_shell = (shell_content_ != nullptr);

    if (in_shell) {
        // Shell mode: destroy old content inside content_area, then create
        // new content as a child. No lv_screen_load — the shell root stays.
        destroy_active();

        lv_group_t *group  = nullptr;
        lv_obj_t   *screen = entry->create_fn(ctx, params, &group);
        if (screen == nullptr) {
            BLUSYS_LOGW(TAG, "create_fn returned null for route %lu",
                        static_cast<unsigned long>(route_id));
            return;
        }

        // Re-parent screen content into the shell's content area if the
        // create_fn made a standalone screen. Products using
        // page_create_in(shell_content) already parent correctly — this
        // handles the case where they used page_create() instead.
        if (lv_obj_get_parent(screen) != shell_content_) {
            lv_obj_set_parent(screen, shell_content_);
        }
        lv_obj_set_size(screen, LV_PCT(100), LV_PCT(100));

        active_screen_     = screen;
        active_destroy_fn_ = entry->destroy_fn;
        active_lifecycle_  = entry->lifecycle;

        // Wire focus.
        if (focus_scope_ != nullptr) {
            focus_scope_->reset();
            focus_scope_->push_scope(screen);
        } else if (group != nullptr) {
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

        fire_show_hooks(screen, entry->lifecycle);
        return;
    }

    // Standalone screen mode: destroy/animate the old screen and load a new one.
    const auto spec = blusys::ui::resolve_transition(transition_field, default_transition);
    const bool animated = (spec.type != blusys::ui::transition_type::none);

    if (animated) {
        // When animated, call the product's custom destroy_fn now (before
        // LVGL auto-deletes the old screen obj after the animation). This
        // ensures product-side cleanup runs even for animated transitions.
        if (active_screen_ != nullptr && active_destroy_fn_ != nullptr) {
            active_destroy_fn_(active_screen_);
            // Clear destroy_fn so destroy_active won't double-call it.
            active_destroy_fn_ = nullptr;
        }
        // Let LVGL delete the lv_obj via auto_del after animation.
        active_screen_     = nullptr;
        active_lifecycle_  = {};
    } else {
        destroy_active();
    }

    // Create the incoming screen.
    lv_group_t *group  = nullptr;
    lv_obj_t   *screen = entry->create_fn(ctx, params, &group);
    if (screen == nullptr) {
        BLUSYS_LOGW(TAG, "create_fn returned null for route %lu",
                    static_cast<unsigned long>(route_id));
        return;
    }

    active_screen_     = screen;
    active_destroy_fn_ = entry->destroy_fn;
    active_lifecycle_  = entry->lifecycle;

    if (animated) {
        lv_screen_load_anim(screen, spec.lv_anim, spec.duration, 0, true);
    } else {
        lv_screen_load(screen);
    }

    // Wire focus.
    if (focus_scope_ != nullptr) {
        focus_scope_->reset();
        focus_scope_->push_scope(screen);
    } else if (group != nullptr) {
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

    // Fire lifecycle hooks on the incoming screen.
    fire_show_hooks(screen, entry->lifecycle);
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
    active_lifecycle_  = {};
}

void screen_registry::fire_hide_hooks()
{
    if (active_screen_ == nullptr) {
        return;
    }
    if (active_lifecycle_.on_focus_lost != nullptr) {
        active_lifecycle_.on_focus_lost(active_screen_, active_lifecycle_.user_data);
    }
    if (active_lifecycle_.on_hide != nullptr) {
        active_lifecycle_.on_hide(active_screen_, active_lifecycle_.user_data);
    }
}

void screen_registry::fire_show_hooks(lv_obj_t *screen, const screen_lifecycle &lifecycle)
{
    if (lifecycle.on_show != nullptr) {
        lifecycle.on_show(screen, lifecycle.user_data);
    }
    if (lifecycle.on_focus_gained != nullptr) {
        lifecycle.on_focus_gained(screen, lifecycle.user_data);
    }
}

}  // namespace blusys::app::view

#endif  // BLUSYS_FRAMEWORK_HAS_UI
