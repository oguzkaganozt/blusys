#pragma once

#include "blusys/hal/error.h"
#include "blusys/framework/feedback/feedback.hpp"
#include "blusys/framework/engine/intent.hpp"
#include "blusys/framework/engine/router.hpp"
#include "blusys/framework/observe/budget.hpp"

#include <cstddef>
#include <cstdint>

namespace blusys {

/// Type-erased app hook for the framework runtime (replaces the old `controller`
/// virtual base). Routes are applied synchronously via `routes` passed to
/// `handle_event`; there is no internal route queue.
struct runtime_handler {
    void *context = nullptr;

    /// Optional; called during `runtime::init` after feedback bus is ready.
    blusys_err_t (*on_init)(void *context, feedback_bus *feedback) = nullptr;

    /// Required; processes one dequeued framework event.
    void (*handle_event)(void *context,
                         const app_event &event,
                         feedback_bus *feedback,
                         route_sink *routes) = nullptr;

    /// Optional; periodic tick from `runtime::step`.
    void (*on_tick)(void *context, std::uint32_t now_ms) = nullptr;

    /// Optional; called from `runtime::deinit`.
    void (*on_deinit)(void *context) = nullptr;
};

class runtime {
public:
    static constexpr std::size_t kMaxQueuedEvents = blusys::budget::event_queue_depth;
    static constexpr std::uint32_t kDefaultTickPeriodMs = 10;

    runtime() = default;

    blusys_err_t init(route_sink *output_route_sink,
                      const runtime_handler &handler,
                      std::uint32_t tick_period_ms = kDefaultTickPeriodMs);
    void deinit();

    blusys_err_t post_event(const app_event &event);
    blusys_err_t post_intent(intent value,
                             std::uint32_t source_id = 0,
                             const void *payload = nullptr);

    bool register_feedback_sink(feedback_sink *sink);
    bool unregister_feedback_sink(feedback_sink *sink);

    void step(std::uint32_t now_ms);

    [[nodiscard]] std::size_t queued_event_count() const;
    [[nodiscard]] std::uint32_t tick_period_ms() const;
    [[nodiscard]] bool is_initialized() const;

    /// Feedback bus used by registered sinks and passed to `runtime_handler` callbacks.
    [[nodiscard]] feedback_bus *feedback_bus_ptr() { return &feedback_bus_; }
    [[nodiscard]] const feedback_bus *feedback_bus_ptr() const { return &feedback_bus_; }

private:
    runtime_handler      handler_{};
    route_sink          *output_route_sink_ = nullptr;
    feedback_bus         feedback_bus_{};
    blusys::ring_buffer<app_event, kMaxQueuedEvents> event_queue_{};
    std::uint32_t        tick_period_ms_ = kDefaultTickPeriodMs;
    std::uint32_t        last_tick_ms_ = 0;
    bool                   initialized_ = false;
    bool                   tick_started_ = false;
};

}  // namespace blusys
