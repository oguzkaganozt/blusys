#pragma once

#include "blusys/error.h"
#include "blusys/framework/core/controller.hpp"
#include "blusys/framework/core/feedback.hpp"
#include "blusys/framework/core/intent.hpp"
#include "blusys/framework/core/router.hpp"

#include <cstddef>
#include <cstdint>

namespace blusys::framework {

class runtime final : public route_sink {
public:
    static constexpr std::size_t kMaxQueuedEvents = 16;
    static constexpr std::size_t kMaxQueuedRoutes = 8;
    static constexpr std::uint32_t kDefaultTickPeriodMs = 10;

    runtime() = default;

    blusys_err_t init(controller *controller,
                      route_sink *output_route_sink = nullptr,
                      std::uint32_t tick_period_ms = kDefaultTickPeriodMs);
    void deinit();

    blusys_err_t post_event(const app_event &event);
    blusys_err_t post_intent(intent value,
                             std::uint32_t source_id = 0,
                             const void *payload = nullptr);

    bool register_feedback_sink(feedback_sink *sink);
    bool unregister_feedback_sink(feedback_sink *sink);

    void step(std::uint32_t now_ms);

    [[nodiscard]] bool poll_route(route_command *out_command);
    [[nodiscard]] std::size_t queued_event_count() const;
    [[nodiscard]] std::size_t queued_route_count() const;
    [[nodiscard]] std::uint32_t tick_period_ms() const;
    [[nodiscard]] bool is_initialized() const;

    void submit(const route_command &command) override;

private:
    void flush_routes();

    controller *controller_ = nullptr;
    route_sink *output_route_sink_ = nullptr;
    feedback_bus feedback_bus_{};
    blusys::ring_buffer<app_event, kMaxQueuedEvents> event_queue_{};
    blusys::ring_buffer<route_command, kMaxQueuedRoutes> route_queue_{};
    std::uint32_t tick_period_ms_ = kDefaultTickPeriodMs;
    std::uint32_t last_tick_ms_ = 0;
    bool initialized_ = false;
    bool tick_started_ = false;
};

}  // namespace blusys::framework
