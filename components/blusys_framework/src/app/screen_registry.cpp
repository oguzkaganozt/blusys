#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/view/screen_registry.hpp"
#include "blusys/app/app_ctx.hpp"
#include "blusys/framework/ui/input/focus_scope.hpp"
#include "blusys/framework/ui/transition.hpp"
#include "blusys/log.h"

static const char *TAG = "screen_reg";

namespace blusys::app::view {

std::uint32_t screen_registry::nav_route_at(std::size_t index_from_bottom) const
{
    if (index_from_bottom >= nav_stack_.size()) {
        return 0;
    }
    return nav_stack_[index_from_bottom].route_id;
}

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
    case T::set_root: {
        if (find(cmd.id) == nullptr) {
            BLUSYS_LOGW(TAG, "set_root: unknown route %lu",
                        static_cast<unsigned long>(cmd.id));
            return false;
        }
        nav_stack_.clear();
        (void)nav_stack_.push_back({.route_id = cmd.id, .params = cmd.params});
        if (!load_screen(cmd.id, cmd.params, ctx, cmd.transition,
                         blusys::ui::transition_type::none)) {
            nav_stack_.clear();
            return false;
        }
        return true;
    }

    case T::push: {
        if (find(cmd.id) == nullptr) {
            BLUSYS_LOGW(TAG, "push: unknown route %lu",
                        static_cast<unsigned long>(cmd.id));
            return false;
        }
        if (nav_stack_.size() >= kMaxStackDepth) {
            BLUSYS_LOGW(TAG, "nav stack full, push ignored");
            return false;
        }
        (void)nav_stack_.push_back({.route_id = cmd.id, .params = cmd.params});
        if (!load_screen(cmd.id, cmd.params, ctx, cmd.transition,
                         blusys::ui::transition_type::slide_left)) {
            nav_stack_.pop_back();
            return false;
        }
        return true;
    }

    case T::replace: {
        if (find(cmd.id) == nullptr) {
            BLUSYS_LOGW(TAG, "replace: unknown route %lu",
                        static_cast<unsigned long>(cmd.id));
            return false;
        }
        nav_entry old_top{};
        const bool had_stack = !nav_stack_.empty();
        if (had_stack) {
            old_top = nav_stack_[nav_stack_.size() - 1];
            nav_stack_[nav_stack_.size() - 1] = {.route_id = cmd.id, .params = cmd.params};
        } else {
            (void)nav_stack_.push_back({.route_id = cmd.id, .params = cmd.params});
        }
        if (!load_screen(cmd.id, cmd.params, ctx, cmd.transition,
                         blusys::ui::transition_type::none)) {
            if (had_stack) {
                nav_stack_[nav_stack_.size() - 1] = old_top;
            } else {
                nav_stack_.clear();
            }
            return false;
        }
        return true;
    }

    case T::pop:
        if (nav_stack_.size() > 1) {
            const nav_entry popped = nav_stack_[nav_stack_.size() - 1];
            nav_stack_.pop_back();
            const auto &prev = nav_stack_[nav_stack_.size() - 1];
            if (!load_screen(prev.route_id, prev.params, ctx, cmd.transition,
                             blusys::ui::transition_type::slide_right)) {
                (void)nav_stack_.push_back(popped);
                BLUSYS_LOGW(TAG, "pop: failed to restore route %lu",
                            static_cast<unsigned long>(prev.route_id));
                return false;
            }
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

bool screen_registry::load_screen(std::uint32_t route_id,
                                   const void   *params,
                                   app_ctx      &ctx,
                                   std::uint32_t transition_field,
                                   blusys::ui::transition_type default_transition)
{
    const screen_entry *entry = find(route_id);
    if (entry == nullptr) {
        BLUSYS_LOGW(TAG, "no screen registered for route %lu",
                    static_cast<unsigned long>(route_id));
        return false;
    }

    // Fire lifecycle hooks on the outgoing screen.
    fire_hide_hooks();

    const bool in_shell = (shell_content_ != nullptr);

    if (in_shell) {
        const auto shell_spec =
            blusys::ui::resolve_transition(transition_field, default_transition);
        const bool shell_animated = (shell_spec.type != blusys::ui::transition_type::none);

        destroy_active();

        lv_group_t *group  = nullptr;
        lv_obj_t   *screen = entry->create_fn(ctx, params, &group);
        if (screen == nullptr) {
            BLUSYS_LOGW(TAG, "create_fn returned null for route %lu",
                        static_cast<unsigned long>(route_id));
            return false;
        }

        if (lv_obj_get_parent(screen) != shell_content_) {
            lv_obj_set_parent(screen, shell_content_);
        }
        lv_obj_set_size(screen, LV_PCT(100), LV_PCT(100));

        active_screen_     = screen;
        active_destroy_fn_ = entry->destroy_fn;
        active_lifecycle_  = entry->lifecycle;

        if (shell_animated) {
            blusys::ui::shell_content_enter_anim(screen, shell_spec);
        } else {
            lv_obj_set_style_opa(screen, LV_OPA_COVER, 0);
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

        fire_show_hooks(screen, entry->lifecycle);
        return true;
    }

    // Standalone screen mode: destroy/animate the old screen and load a new one.
    const auto spec = blusys::ui::resolve_transition(transition_field, default_transition);
    const bool animated = (spec.type != blusys::ui::transition_type::none);

    if (animated) {
        if (active_screen_ != nullptr && active_destroy_fn_ != nullptr) {
            active_destroy_fn_(active_screen_);
            active_destroy_fn_ = nullptr;
        }
        active_screen_    = nullptr;
        active_lifecycle_ = {};
    } else {
        destroy_active();
    }

    lv_group_t *group  = nullptr;
    lv_obj_t   *screen = entry->create_fn(ctx, params, &group);
    if (screen == nullptr) {
        BLUSYS_LOGW(TAG, "create_fn returned null for route %lu",
                    static_cast<unsigned long>(route_id));
        return false;
    }

    active_screen_     = screen;
    active_destroy_fn_ = entry->destroy_fn;
    active_lifecycle_  = entry->lifecycle;

    if (animated) {
        lv_screen_load_anim(screen, spec.lv_anim, spec.duration, 0, true);
    } else {
        lv_screen_load(screen);
    }

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
    return true;
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
