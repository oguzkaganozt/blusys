#include "blusys/framework/core/runtime.hpp"

#include "blusys/log.h"

namespace blusys::framework {
namespace {

constexpr const char *kTag = "blusys_runtime";

}  // namespace

blusys_err_t runtime::init(route_sink *output_route_sink,
                           const runtime_handler &handler,
                           std::uint32_t tick_period_ms)
{
    if (output_route_sink == nullptr || tick_period_ms == 0 ||
        handler.handle_event == nullptr) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    if (initialized_) {
        return BLUSYS_ERR_INVALID_STATE;
    }

    handler_           = handler;
    output_route_sink_ = output_route_sink;
    tick_period_ms_    = tick_period_ms;
    last_tick_ms_      = 0;
    tick_started_      = false;
    event_queue_.clear();

    blusys_err_t err = BLUSYS_OK;
    if (handler_.on_init != nullptr) {
        err = handler_.on_init(handler_.context, &feedback_bus_);
    }
    if (err != BLUSYS_OK) {
        handler_           = {};
        output_route_sink_ = nullptr;
        return err;
    }

    initialized_ = true;
    return BLUSYS_OK;
}

void runtime::deinit()
{
    if (!initialized_) {
        return;
    }

    if (handler_.on_deinit != nullptr) {
        handler_.on_deinit(handler_.context);
    }

    handler_           = {};
    output_route_sink_ = nullptr;
    event_queue_.clear();
    initialized_       = false;
    tick_started_      = false;
}

blusys_err_t runtime::post_event(const app_event &event)
{
    if (!initialized_) {
        return BLUSYS_ERR_INVALID_STATE;
    }

    if (!event_queue_.push_back(event)) {
        BLUSYS_LOGW(kTag, "event queue full");
        return BLUSYS_ERR_BUSY;
    }

    return BLUSYS_OK;
}

blusys_err_t runtime::post_intent(intent value,
                                  std::uint32_t source_id,
                                  const void *payload)
{
    return post_event(make_intent_event(value, source_id, payload));
}

bool runtime::register_feedback_sink(feedback_sink *sink)
{
    return feedback_bus_.register_sink(sink);
}

bool runtime::unregister_feedback_sink(feedback_sink *sink)
{
    return feedback_bus_.unregister_sink(sink);
}

void runtime::step(std::uint32_t now_ms)
{
    if (!initialized_) {
        return;
    }

    app_event event;
    while (event_queue_.pop_front(&event)) {
        handler_.handle_event(handler_.context, event, &feedback_bus_, output_route_sink_);
    }

    if (!tick_started_ || (now_ms - last_tick_ms_) >= tick_period_ms_) {
        if (handler_.on_tick != nullptr) {
            handler_.on_tick(handler_.context, now_ms);
        }
        tick_started_ = true;
        last_tick_ms_ = now_ms;
    }
}

std::size_t runtime::queued_event_count() const
{
    return event_queue_.size();
}

std::uint32_t runtime::tick_period_ms() const
{
    return tick_period_ms_;
}

bool runtime::is_initialized() const
{
    return initialized_;
}

}  // namespace blusys::framework
