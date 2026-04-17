#pragma once

// Internal bridge between blusys::app and blusys::framework.
// Product code should never include this header directly.

#include "blusys/framework/app/ctx.hpp"
#include "blusys/framework/app/spec.hpp"
#include "blusys/framework/capabilities/persistence.hpp"
#include "blusys/framework/flows/flow_base.hpp"
#include "blusys/framework/capabilities/list.hpp"
#include "blusys/framework/engine/internal/default_route_sink.hpp"
#include "blusys/framework/containers.hpp"
#include "blusys/framework/feedback/feedback.hpp"
#include "blusys/framework/engine/intent.hpp"
#include "blusys/framework/engine/router.hpp"
#include "blusys/framework/engine/event_queue.hpp"
#include "blusys/framework/feedback/internal/logging_sink.hpp"
#include "blusys/framework/observe/budget.hpp"
#include "blusys/hal/log.h"

#include <cstddef>
#include <cstdint>

#ifdef BLUSYS_FRAMEWORK_HAS_UI
#include "blusys/framework/ui/composition/screen_router.hpp"
#include "blusys/framework/ui/composition/shell.hpp"
#include "blusys/framework/ui/controller/navigation_controller.hpp"
#endif

namespace blusys {

class storage_capability;
class persistence_capability;

// ---- non-template base: wires app_ctx internals ----

class app_runtime_base {
protected:
    static void record_action_queue_drop(app_ctx &ctx)
    {
        ++ctx.action_queue_drop_count_;
    }

    static void bind_ctx(app_ctx &ctx,
                         blusys::feedback_bus *feedback_bus,
                         app_ctx::dispatch_fn dispatch_fn,
                         void *runtime_ptr)
    {
        ctx.feedback_bus_ = feedback_bus;
        ctx.dispatch_fn_  = dispatch_fn;
        ctx.runtime_ptr_  = runtime_ptr;
    }

    static void bind_product_state(app_ctx &ctx, void *state)
    {
        ctx.product_state_ = state;
    }

    static void bind_services_ptr(app_ctx &ctx, app_services *svc)
    {
        ctx.services_ = svc;
    }

#ifdef BLUSYS_FRAMEWORK_HAS_UI
    static void bind_fx_navigation(app_ctx &ctx, navigation_controller *ctrl)
    {
        ctx.fx_.bind_navigation(ctrl);
    }
#endif

    static void bind_fx_storage(app_ctx &ctx, storage_capability *storage)
    {
        ctx.fx_.bind_storage(storage);
    }

    static void bind_fx_persistence(app_ctx &ctx, persistence_capability *persistence)
    {
        ctx.fx_.bind_persistence(persistence);
    }

    static void wire_route_sink(app_services &svc, blusys::route_sink *sink)
    {
        svc.route_sink_ = sink;
    }

#ifdef BLUSYS_FRAMEWORK_HAS_UI
    static void wire_screen_router(app_services &svc, screen_router *router)
    {
        svc.screen_router_ = router;
    }

    static void wire_shell(app_services &svc, shell *shell)
    {
        svc.shell_ = shell;
    }
#endif

    // Defined in ctx.cpp so storage.hpp does not leak into every header
    // that pulls app_runtime.hpp.
    static void sync_services_storage(app_ctx &ctx, app_services &svc);

    static void bind_capabilities(app_ctx &ctx, capability_list *capabilities)
    {
        ctx.bind_capabilities(capabilities);
    }
};

// ---- app_runtime: the core bridge template ----

template <typename State, typename Action,
          std::size_t ActionQueueCap = blusys::budget::action_queue_depth>
class app_runtime final : public app_runtime_base {
public:
    using spec_type = app_spec<State, Action>;

    explicit app_runtime(const spec_type &spec)
        : spec_(spec), state_(spec.initial_state)
    {
    }

    blusys_err_t init()
    {
        feedback_logging_sink_.set_identity(spec_.identity);
        framework_runtime_.register_feedback_sink(&feedback_logging_sink_);

        blusys::runtime_handler handler{};
        handler.context      = this;
        handler.on_init      = &app_runtime::reducer_on_init;
        handler.handle_event = &app_runtime::reducer_handle_event;
        handler.on_tick      = &app_runtime::reducer_on_tick;
        handler.on_deinit    = nullptr;

        blusys_err_t err = framework_runtime_.init(
            &route_sink_ref(), handler, spec_.tick_period_ms);
        if (err != BLUSYS_OK) {
            return err;
        }

        bind_services_ptr(ctx_, &services_);
        wire_route_sink(services_, &route_sink_ref());

#ifdef BLUSYS_FRAMEWORK_HAS_UI
        wire_screen_router(services_, &screen_router_);
        screen_router_.bind_ctx(&ctx_);

        if (spec_.shell != nullptr) {
            shell_ = shell_create(*spec_.shell);
            wire_shell(services_, &shell_);
            shell_load(shell_);
            screen_router_.bind_shell(shell_.content_area);
            shell_.router = &screen_router_;
            controller_.bind(&screen_router_, &shell_);
        } else {
            controller_.bind(&screen_router_, nullptr);
        }
        bind_fx_navigation(ctx_, &controller_);
#endif

        bind_capabilities(ctx_, spec_.capabilities);
        bind_fx_storage(ctx_, ctx_.get<storage_capability>());
        bind_fx_persistence(ctx_, ctx_.get<persistence_capability>());
        start_capabilities();
        sync_services_storage(ctx_, services_);
        bind_product_state(ctx_, static_cast<void *>(&state_));

        start_flows();

        if (spec_.on_init != nullptr) {
            spec_.on_init(ctx_, ctx_.fx(), state_);
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
        bind_product_state(ctx_, nullptr);
        stop_flows();
        stop_capabilities();
        framework_runtime_.deinit();
        framework_runtime_.unregister_feedback_sink(&feedback_logging_sink_);
    }

    bool post_action(const Action &action)
    {
        if (!action_queue_.push_back(action)) {
            record_action_queue_drop(ctx_);
            BLUSYS_LOGW("blusys_app", "action queue full, dispatch dropped");
            return false;
        }
        return true;
    }

    void post_intent(blusys::intent intent,
                     std::uint32_t source_id = 0,
                     const void *payload = nullptr)
    {
        framework_runtime_.post_intent(intent, source_id, payload);
    }

    [[nodiscard]] blusys::runtime &framework_runtime()
    {
        return framework_runtime_;
    }

    [[nodiscard]] const State &state() const { return state_; }
    [[nodiscard]] State &state() { return state_; }
    [[nodiscard]] app_ctx &ctx() { return ctx_; }

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

    void start_flows()
    {
#ifdef BLUSYS_FRAMEWORK_HAS_UI
        if (spec_.flows == nullptr) {
            return;
        }
        for (std::size_t i = 0; i < spec_.flow_count; ++i) {
            if (spec_.flows[i] != nullptr) {
                spec_.flows[i]->start(ctx_);
            }
        }
#endif
    }

    void stop_flows()
    {
#ifdef BLUSYS_FRAMEWORK_HAS_UI
        if (spec_.flows == nullptr) {
            return;
        }
        for (std::size_t i = spec_.flow_count; i > 0; --i) {
            if (spec_.flows[i - 1] != nullptr) {
                spec_.flows[i - 1]->stop();
            }
        }
#endif
    }

    void start_capabilities()
    {
        if (spec_.capabilities == nullptr) {
            return;
        }

        // Persistence starts first: it owns NVS init and runs schema migrations
        // for all other capabilities before they read their persisted state.
        auto* persistence = ctx_.get<persistence_capability>();
        if (persistence != nullptr) {
            blusys_err_t err = persistence->start(framework_runtime_);
            if (err != BLUSYS_OK) {
                BLUSYS_LOGW("blusys_app", "persistence capability start failed: %d",
                            static_cast<int>(err));
            } else {
                persistence->run_migrations(*spec_.capabilities);
            }
        }

        for (std::size_t i = 0; i < spec_.capabilities->count; ++i) {
            capability_base *c = spec_.capabilities->items[i];
            if (c == nullptr || c == persistence) {
                continue;  // skip persistence (already started above)
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
        // Stop in reverse order; persistence stops last (symmetrical with start).
        auto* persistence = ctx_.get<persistence_capability>();
        for (std::size_t i = spec_.capabilities->count; i > 0; --i) {
            capability_base *c = spec_.capabilities->items[i - 1];
            if (c != nullptr && c != persistence) {
                c->stop();
            }
        }
        if (persistence != nullptr) {
            persistence->stop();
        }
    }

    static bool dispatch_trampoline(void *self, const void *action_ptr)
    {
        auto *rt = static_cast<app_runtime *>(self);
        const auto &action = *static_cast<const Action *>(action_ptr);
        return rt->post_action(action);
    }

    static blusys_err_t reducer_on_init(void * /*ctx*/, blusys::feedback_bus * /*fb*/)
    {
        return BLUSYS_OK;
    }

    static void reducer_handle_event(void *ctx,
                                     const blusys::app_event &event,
                                     blusys::feedback_bus * /*fb*/,
                                     blusys::route_sink * /*routes*/)
    {
        auto *owner = static_cast<app_runtime *>(ctx);
        if (owner->spec_.on_event != nullptr) {
            if (auto out_action = owner->spec_.on_event(event, owner->state_);
                out_action.has_value()) {
                owner->post_action(*out_action);
            }
            return;
        }
    }

    static void reducer_on_tick(void *ctx, std::uint32_t now_ms)
    {
        auto *owner = static_cast<app_runtime *>(ctx);
        if (owner->spec_.on_tick != nullptr) {
            owner->spec_.on_tick(owner->ctx_, owner->ctx_.fx(), owner->state_, now_ms);
        }
    }

    blusys::route_sink &route_sink_ref()
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
    app_services                                         services_{};
    blusys::runtime                           framework_runtime_{};
    blusys::ring_buffer<Action, ActionQueueCap>          action_queue_{};
    detail::feedback_logging_sink                        feedback_logging_sink_{};
#ifdef BLUSYS_FRAMEWORK_HAS_UI
    class screen_router       screen_router_{};
    struct shell              shell_{};
    navigation_controller     controller_{};
#else
    detail::default_route_sink                           default_route_sink_{};
#endif
};

}  // namespace blusys
