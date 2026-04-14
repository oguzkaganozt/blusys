#pragma once

// Internal bridge between blusys::app and blusys::framework.
// Product code should never include this header directly.

#include "blusys/framework/app/ctx.hpp"
#include "blusys/framework/app/spec.hpp"
#include "blusys/framework/capabilities/list.hpp"
#include "blusys/framework/engine/internal/default_route_sink.hpp"
#include "blusys/framework/containers.hpp"
#include "blusys/framework/feedback/feedback.hpp"
#include "blusys/framework/engine/intent.hpp"
#include "blusys/framework/engine/router.hpp"
#include "blusys/framework/engine/event_queue.hpp"
#include "blusys/framework/feedback/internal/logging_sink.hpp"
#include "blusys/hal/log.h"

#include <cstddef>
#include <cstdint>
#include <type_traits>

namespace blusys::detail {

template <typename T, typename = void>
struct action_has_cap_event_member : std::false_type {};

template <typename T>
struct action_has_cap_event_member<T, std::void_t<decltype(std::declval<T &>().cap_event)>>
    : std::true_type {};

template <typename T>
inline constexpr bool action_has_cap_event_member_v = action_has_cap_event_member<T>::value;

}  // namespace blusys::detail

#ifdef BLUSYS_FRAMEWORK_HAS_UI
#include "blusys/framework/ui/composition/screen_router.hpp"
#include "blusys/framework/ui/composition/shell.hpp"
#endif

namespace blusys {

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

    static void sync_services_storage(app_ctx &ctx, app_services &svc)
    {
        svc.set_storage_for_fs(ctx.storage_);
    }

    static void bind_capability_ptrs(app_ctx &ctx, capability_list *capabilities)
    {
        app_ctx::bind_capability_pointers_from_list(ctx, capabilities);
    }
};

// ---- app_runtime: the core bridge template ----

template <typename State, typename Action, std::size_t ActionQueueCap = 16>
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
#endif

        bind_ctx(ctx_, framework_runtime_.feedback_bus_ptr(), &dispatch_trampoline, this);

#ifdef BLUSYS_FRAMEWORK_HAS_UI
        // Create and load the interaction shell if configured.
        if (spec_.shell != nullptr) {
            shell_ = shell_create(*spec_.shell);
            shell_.svc = &services_;
            wire_shell(services_, &shell_);
            shell_load(shell_);

            // Tell the screen_registry to swap content inside the shell's
            // content area instead of loading standalone screens.
            screen_router_.bind_shell(shell_.content_area);

            // Auto-update shell chrome (back button, tab highlight, title) on every transition.
            screen_router_.set_screen_changed_callback(
                [](lv_obj_t * /*screen*/, void *user_ctx) {
                    auto *s = static_cast<shell *>(user_ctx);
                    if (s == nullptr || s->svc == nullptr) {
                        return;
                    }
                    auto *router = s->svc->screen_router();
                    if (router == nullptr) {
                        return;
                    }
                    router->sync_shell_chrome(*s);
                    shell_set_back_visible(*s, router->stack_depth() > 1);
                },
                &shell_);
        }
#endif

        start_capabilities();
        bind_capability_ptrs(ctx_, spec_.capabilities);
        sync_services_storage(ctx_, services_);
        bind_product_state(ctx_, static_cast<void *>(&state_));

        if (spec_.capabilities != nullptr && spec_.map_event == nullptr) {
            if constexpr (!detail::action_has_cap_event_member_v<Action>) {
                BLUSYS_LOGE("blusys_app",
                            "capabilities require app_spec::map_event when Action has no "
                            "cap_event field");
                return BLUSYS_ERR_INVALID_ARG;
            }
            if constexpr (detail::action_has_cap_event_member_v<Action>) {
                if (spec_.capability_event_discriminant == k_capability_event_discriminant_unset) {
                    BLUSYS_LOGE(
                        "blusys_app",
                        "capabilities registered but capability_event_discriminant is unset");
                    return BLUSYS_ERR_INVALID_ARG;
                }
            }
        }

        if (spec_.on_init != nullptr) {
            spec_.on_init(ctx_, services_, state_);
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

#ifdef BLUSYS_FRAMEWORK_HAS_UI
    [[nodiscard]] screen_router &screen_router() { return screen_router_; }
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
        if (event.kind == blusys::app_event_kind::intent) {
            if (owner->spec_.map_intent == nullptr) {
                return;
            }
            Action action;
            if (owner->spec_.map_intent(owner->services_,
                                        blusys::app_event_intent(event),
                                        &action)) {
                owner->post_action(action);
            }
        } else if (event.kind == blusys::app_event_kind::integration) {
            capability_event ce{};
            const bool mapped =
                map_integration_event(event.id, event.code, &ce);
            if (mapped) {
                ce.payload = event.payload;
            } else {
                if (owner->spec_.map_event == nullptr) {
                    if (owner->spec_.capabilities != nullptr) {
                        BLUSYS_LOGW("blusys_app",
                                    "unknown integration event id=%lu code=%lu (no map_event)",
                                    static_cast<unsigned long>(event.id),
                                    static_cast<unsigned long>(event.code));
                    }
                    return;
                }
                ce.tag            = capability_event_tag::integration_passthrough;
                ce.value          = 0;
                ce.raw_event_id   = event.id;
                ce.raw_event_code = event.code;
                ce.payload        = event.payload;
            }

            if (owner->spec_.map_event != nullptr) {
                Action out_action{};
                if (owner->spec_.map_event(ce, &out_action)) {
                    owner->post_action(out_action);
                } else {
                    BLUSYS_LOGD("blusys_app",
                                "map_event dropped integration event (tag=%u)",
                                static_cast<unsigned>(ce.tag));
                }
                return;
            }

            if (!mapped || ce.tag == capability_event_tag::integration_passthrough) {
                return;
            }

            if (owner->spec_.capability_event_discriminant ==
                k_capability_event_discriminant_unset) {
                return;
            }
            if constexpr (detail::action_has_cap_event_member_v<Action>) {
                Action out_action{};
                using tag_type =
                    std::remove_const_t<std::remove_reference_t<decltype(out_action.tag)>>;
                out_action.tag =
                    static_cast<tag_type>(owner->spec_.capability_event_discriminant);
                out_action.cap_event = ce;
                owner->post_action(out_action);
            }
        }
    }

    static void reducer_on_tick(void *ctx, std::uint32_t now_ms)
    {
        auto *owner = static_cast<app_runtime *>(ctx);
        if (owner->spec_.on_tick != nullptr) {
            owner->spec_.on_tick(owner->ctx_, owner->services_, owner->state_, now_ms);
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
    screen_router                                  screen_router_{};
    shell                                          shell_{};
#else
    detail::default_route_sink                           default_route_sink_{};
#endif
};

}  // namespace blusys
