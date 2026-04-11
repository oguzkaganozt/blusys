#pragma once

// Internal bridge between blusys::app and blusys::framework.
// Product code should never include this header directly.

#include "blusys/app/app_ctx.hpp"
#include "blusys/app/app_spec.hpp"
#include "blusys/app/capability_list.hpp"
#include "blusys/app/capabilities/connectivity.hpp"
#include "blusys/app/capabilities/storage.hpp"
#include "blusys/app/capabilities/bluetooth.hpp"
#include "blusys/app/capabilities/ota.hpp"
#include "blusys/app/capabilities/diagnostics.hpp"
#include "blusys/app/capabilities/telemetry.hpp"
#include "blusys/app/capabilities/provisioning.hpp"
#include "blusys/framework/core/containers.hpp"
#include "blusys/framework/core/controller.hpp"
#include "blusys/framework/core/feedback.hpp"
#include "blusys/framework/core/intent.hpp"
#include "blusys/framework/core/router.hpp"
#include "blusys/framework/core/runtime.hpp"
#include "blusys/app/detail/feedback_logging_sink.hpp"
#include "blusys/log.h"

#include <cstddef>
#include <cstdint>

#ifdef BLUSYS_FRAMEWORK_HAS_UI
#include "blusys/app/view/screen_router.hpp"
#include "blusys/app/view/shell.hpp"
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
    static void bind_screen_router(app_ctx &ctx, view::screen_router *router)
    {
        ctx.screen_router_ = router;
    }

    static void bind_shell(app_ctx &ctx, view::shell *shell)
    {
        ctx.shell_ = shell;
    }
#endif

    static void bind_capability_ptrs(app_ctx &ctx, capability_list *capabilities)
    {
        if (capabilities == nullptr) {
            return;
        }
        for (std::size_t i = 0; i < capabilities->count; ++i) {
            capability_base *c = capabilities->items[i];
            if (c == nullptr) {
                continue;
            }
            switch (c->kind()) {
            case capability_kind::connectivity:
                ctx.connectivity_ = static_cast<connectivity_capability *>(c);
                break;
            case capability_kind::storage:
                ctx.storage_ = static_cast<storage_capability *>(c);
                break;
            case capability_kind::bluetooth:
                ctx.bluetooth_ = static_cast<bluetooth_capability *>(c);
                break;
            case capability_kind::ota:
                ctx.ota_ = static_cast<ota_capability *>(c);
                break;
            case capability_kind::diagnostics:
                ctx.diagnostics_ = static_cast<diagnostics_capability *>(c);
                break;
            case capability_kind::telemetry:
                ctx.telemetry_ = static_cast<telemetry_capability *>(c);
                break;
            case capability_kind::provisioning:
                ctx.provisioning_ = static_cast<provisioning_capability *>(c);
                break;
            case capability_kind::custom:
                break;
            }
        }
    }
};

namespace detail {

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
        feedback_logging_sink_.set_identity(spec_.identity);
        framework_runtime_.register_feedback_sink(&feedback_logging_sink_);

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
        bind_screen_router(ctx_, &screen_router_);
        screen_router_.bind_ctx(&ctx_);

        // Create and load the interaction shell if configured.
        if (spec_.shell != nullptr) {
            shell_ = view::shell_create(*spec_.shell);
            shell_.ctx = &ctx_;
            bind_shell(ctx_, &shell_);
            view::shell_load(shell_);

            // Tell the screen_registry to swap content inside the shell's
            // content area instead of loading standalone screens.
            screen_router_.bind_shell(shell_.content_area);

            // Auto-update header on every screen change.
            screen_router_.set_screen_changed_callback(
                [](lv_obj_t * /*screen*/, void *user_ctx) {
                    auto *s = static_cast<view::shell *>(user_ctx);
                    if (s == nullptr || s->ctx == nullptr) {
                        return;
                    }
                    auto *router = s->ctx->screen_router();
                    if (router == nullptr) {
                        return;
                    }
                    view::shell_set_back_visible(*s, router->stack_depth() > 1);
                },
                &shell_);
        }
#endif

        start_capabilities();
        bind_capability_ptrs(ctx_, spec_.capabilities);

        if (spec_.on_init != nullptr) {
            spec_.on_init(ctx_, state_);
        }

        return BLUSYS_OK;
    }

    void step(std::uint32_t now_ms)
    {
        drain_actions();
        poll_capabilities(now_ms);
        framework_runtime_.step(now_ms);
        drain_actions();  // process actions dispatched during tick/handle
    }

    void deinit()
    {
        stop_capabilities();
        framework_runtime_.deinit();
        framework_runtime_.unregister_feedback_sink(&feedback_logging_sink_);
    }

    bool post_action(const Action &action)
    {
        return action_queue_.push_back(action);
    }

    void post_intent(blusys::framework::intent intent,
                     std::uint32_t source_id = 0,
                     const void *payload = nullptr)
    {
        framework_runtime_.post_intent(intent, source_id, payload);
    }

    [[nodiscard]] blusys::framework::runtime &framework_runtime()
    {
        return framework_runtime_;
    }

    [[nodiscard]] const State &state() const { return state_; }
    [[nodiscard]] State &state() { return state_; }
    [[nodiscard]] app_ctx &ctx() { return ctx_; }

#ifdef BLUSYS_FRAMEWORK_HAS_UI
    [[nodiscard]] view::screen_router &screen_router() { return screen_router_; }
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

    void start_capabilities()
    {
        if (spec_.capabilities == nullptr) {
            return;
        }
        for (std::size_t i = 0; i < spec_.capabilities->count; ++i) {
            capability_base *c = spec_.capabilities->items[i];
            if (c == nullptr) {
                continue;
            }
            blusys_err_t err = c->start(framework_runtime_);
            if (err != BLUSYS_OK) {
                BLUSYS_LOGW("blusys_app", "capability start failed: %d",
                            static_cast<int>(err));
            }
        }
    }

    void poll_capabilities(std::uint32_t now_ms)
    {
        if (spec_.capabilities == nullptr) {
            return;
        }
        for (std::size_t i = 0; i < spec_.capabilities->count; ++i) {
            capability_base *c = spec_.capabilities->items[i];
            if (c != nullptr) {
                c->poll(now_ms);
            }
        }
    }

    void stop_capabilities()
    {
        if (spec_.capabilities == nullptr) {
            return;
        }
        for (std::size_t i = spec_.capabilities->count; i > 0; --i) {
            capability_base *c = spec_.capabilities->items[i - 1];
            if (c != nullptr) {
                c->stop();
            }
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
            if (event.kind == blusys::framework::app_event_kind::intent) {
                if (owner_.spec_.map_intent == nullptr) {
                    return;
                }
                Action action;
                if (owner_.spec_.map_intent(blusys::framework::app_event_intent(event), &action)) {
                    owner_.post_action(action);
                }
            } else if (event.kind == blusys::framework::app_event_kind::integration) {
                if (owner_.spec_.map_event == nullptr) {
                    return;
                }
                Action action;
                if (owner_.spec_.map_event(event.id, event.code, event.payload, &action)) {
                    owner_.post_action(action);
                }
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
        return screen_router_;
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
    detail::feedback_logging_sink                        feedback_logging_sink_{};
#ifdef BLUSYS_FRAMEWORK_HAS_UI
    view::screen_router                                  screen_router_{};
    view::shell                                          shell_{};
#else
    detail::default_route_sink                           default_route_sink_{};
#endif
};

}  // namespace blusys::app
