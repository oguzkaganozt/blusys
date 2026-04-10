#pragma once

// Internal bridge between blusys::app and blusys::framework.
// Product code should never include this header directly.

#include "blusys/app/app_ctx.hpp"
#include "blusys/app/app_spec.hpp"
#include "blusys/framework/core/containers.hpp"
#include "blusys/framework/core/controller.hpp"
#include "blusys/framework/core/feedback.hpp"
#include "blusys/framework/core/intent.hpp"
#include "blusys/framework/core/router.hpp"
#include "blusys/framework/core/runtime.hpp"
#include "blusys/log.h"

#include <cstddef>
#include <cstdint>

#ifdef BLUSYS_FRAMEWORK_HAS_UI
#include "blusys/app/view/overlay_manager.hpp"
#endif

namespace blusys::app {

// ---- non-template base: wires app_ctx internals ----

class app_runtime_base {
protected:
    static void bind_ctx(app_ctx &ctx,
                         blusys::framework::route_sink *route_sink,
                         blusys::framework::feedback_bus *feedback_bus,
                         app_ctx::dispatch_fn dispatch_fn,
                         void *runtime_ptr)
    {
        ctx.route_sink_   = route_sink;
        ctx.feedback_bus_ = feedback_bus;
        ctx.dispatch_fn_  = dispatch_fn;
        ctx.runtime_ptr_  = runtime_ptr;
    }

#ifdef BLUSYS_FRAMEWORK_HAS_UI
    static void bind_overlay_manager(app_ctx &ctx, view::overlay_manager *mgr)
    {
        ctx.overlay_mgr_ = mgr;
    }
#endif
};

namespace detail {

// ---- logging feedback sink (registered by default) ----

class default_feedback_sink final : public blusys::framework::feedback_sink {
public:
    bool supports(blusys::framework::feedback_channel) const override
    {
        return true;
    }

    void emit(const blusys::framework::feedback_event &event) override
    {
        BLUSYS_LOGI("blusys_app",
                     "feedback: channel=%s pattern=%s value=%lu",
                     blusys::framework::feedback_channel_name(event.channel),
                     blusys::framework::feedback_pattern_name(event.pattern),
                     static_cast<unsigned long>(event.value));
    }
};

// ---- logging route sink (Phase 1 — logs route commands) ----

class default_route_sink final : public blusys::framework::route_sink {
public:
    void submit(const blusys::framework::route_command &command) override
    {
        BLUSYS_LOGI("blusys_app",
                     "route: %s id=%lu",
                     blusys::framework::route_command_type_name(command.type),
                     static_cast<unsigned long>(command.id));
    }
};

}  // namespace detail

// ---- app_runtime: the core bridge template ----

template <typename State, typename Action, std::size_t ActionQueueCap = 16>
class app_runtime final : public app_runtime_base {
public:
    using spec_type = app_spec<State, Action>;

    explicit app_runtime(const spec_type &spec)
        : spec_(spec)
        , state_(spec.initial_state)
        , reducer_ctrl_(*this)
    {
    }

    blusys_err_t init()
    {
        framework_runtime_.register_feedback_sink(&default_feedback_sink_);

        blusys_err_t err = framework_runtime_.init(
            &reducer_ctrl_, &route_sink_ref(), spec_.tick_period_ms);
        if (err != BLUSYS_OK) {
            return err;
        }

        // Bind app_ctx: the framework runtime itself implements route_sink,
        // so we use it for navigation. Feedback goes through the controller's
        // bound feedback_bus (set by framework_runtime_.init).
        bind_ctx(ctx_,
                 &framework_runtime_,          // route_sink (runtime implements it)
                 reducer_ctrl_.get_feedback_bus(),
                 &dispatch_trampoline,
                 this);

#ifdef BLUSYS_FRAMEWORK_HAS_UI
        bind_overlay_manager(ctx_, &overlay_mgr_);
#endif

        if (spec_.on_init != nullptr) {
            spec_.on_init(ctx_, state_);
        }

        return BLUSYS_OK;
    }

    void step(std::uint32_t now_ms)
    {
        drain_actions();
        framework_runtime_.step(now_ms);
        drain_actions();  // process actions dispatched during tick/handle
    }

    void deinit()
    {
        framework_runtime_.deinit();
        framework_runtime_.unregister_feedback_sink(&default_feedback_sink_);
    }

    bool post_action(const Action &action)
    {
        return action_queue_.push_back(action);
    }

    [[nodiscard]] const State &state() const { return state_; }
    [[nodiscard]] State &state() { return state_; }
    [[nodiscard]] app_ctx &ctx() { return ctx_; }

#ifdef BLUSYS_FRAMEWORK_HAS_UI
    [[nodiscard]] view::overlay_manager &overlay_manager() { return overlay_mgr_; }
#endif

private:
    static constexpr std::size_t kMaxDrainIterations = 32;

    void drain_actions()
    {
        std::size_t iterations = 0;
        Action action;
        while (action_queue_.pop_front(&action) && iterations < kMaxDrainIterations) {
            spec_.update(ctx_, state_, action);
            ++iterations;
        }
    }

    static void dispatch_trampoline(void *self, const void *action_ptr)
    {
        auto *rt = static_cast<app_runtime *>(self);
        const auto &action = *static_cast<const Action *>(action_ptr);
        if (!rt->action_queue_.push_back(action)) {
            BLUSYS_LOGW("blusys_app", "action queue full, dispatch dropped");
        }
    }

    // ---- inner controller: bridges framework events to the reducer model ----

    class reducer_controller final : public blusys::framework::controller {
    public:
        explicit reducer_controller(app_runtime &owner) : owner_(owner) {}

        // Expose the protected accessor for app_runtime::init() to bind app_ctx.
        blusys::framework::feedback_bus *get_feedback_bus() { return bound_feedback_bus(); }

        blusys_err_t init() override
        {
            return BLUSYS_OK;
        }

        void handle(const blusys::framework::app_event &event) override
        {
            if (event.kind != blusys::framework::app_event_kind::intent) {
                return;
            }
            if (owner_.spec_.map_intent == nullptr) {
                return;
            }

            Action action;
            if (owner_.spec_.map_intent(blusys::framework::app_event_intent(event), &action)) {
                owner_.post_action(action);
            }
        }

        void tick(std::uint32_t now_ms) override
        {
            if (owner_.spec_.on_tick != nullptr) {
                owner_.spec_.on_tick(owner_.ctx_, owner_.state_, now_ms);
            }
        }

    private:
        app_runtime &owner_;
    };

    blusys::framework::route_sink &route_sink_ref()
    {
#ifdef BLUSYS_FRAMEWORK_HAS_UI
        return overlay_mgr_;
#else
        return default_route_sink_;
#endif
    }

    spec_type                                            spec_;
    State                                                state_;
    app_ctx                                              ctx_{};
    reducer_controller                                   reducer_ctrl_;
    blusys::framework::runtime                           framework_runtime_{};
    blusys::ring_buffer<Action, ActionQueueCap>          action_queue_{};
    detail::default_feedback_sink                        default_feedback_sink_{};
#ifdef BLUSYS_FRAMEWORK_HAS_UI
    view::overlay_manager                                overlay_mgr_{};
#else
    detail::default_route_sink                           default_route_sink_{};
#endif
};

}  // namespace blusys::app
