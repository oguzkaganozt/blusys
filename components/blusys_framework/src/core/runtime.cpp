#include "blusys/framework/core/runtime.hpp"

#include "blusys/log.h"

namespace blusys::framework {
namespace {

constexpr const char *kTag = "blusys_runtime";

}  // namespace

blusys_err_t runtime::init(controller *controller,
                           route_sink *output_route_sink,
                           std::uint32_t tick_period_ms)
{
    if (controller == nullptr || tick_period_ms == 0) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    if (initialized_) {
        return BLUSYS_ERR_INVALID_STATE;
    }

    controller_ = controller;
    output_route_sink_ = output_route_sink;
    tick_period_ms_ = tick_period_ms;
    last_tick_ms_ = 0;
    tick_started_ = false;
    event_queue_.clear();
    route_queue_.clear();

    controller_->bind_route_sink(this);
    controller_->bind_feedback_bus(&feedback_bus_);

    blusys_err_t err = controller_->init();
    if (err != BLUSYS_OK) {
        controller_->bind_route_sink(nullptr);
        controller_->bind_feedback_bus(nullptr);
        controller_ = nullptr;
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

    controller_->deinit();
    controller_->bind_route_sink(nullptr);
    controller_->bind_feedback_bus(nullptr);

    controller_ = nullptr;
    output_route_sink_ = nullptr;
    event_queue_.clear();
    route_queue_.clear();
    initialized_ = false;
    tick_started_ = false;
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
        controller_->handle(event);
        flush_routes();
    }

    if (!tick_started_ || (now_ms - last_tick_ms_) >= tick_period_ms_) {
        controller_->tick(now_ms);
        tick_started_ = true;
        last_tick_ms_ = now_ms;
        flush_routes();
    }
}

bool runtime::poll_route(route_command *out_command)
{
    return route_queue_.pop_front(out_command);
}

std::size_t runtime::queued_event_count() const
{
    return event_queue_.size();
}

std::size_t runtime::queued_route_count() const
{
    return route_queue_.size();
}

std::uint32_t runtime::tick_period_ms() const
{
    return tick_period_ms_;
}

bool runtime::is_initialized() const
{
    return initialized_;
}

void runtime::submit(const route_command &command)
{
    if (!route_queue_.push_back(command)) {
        BLUSYS_LOGW(kTag, "route queue full");
    }
}

void runtime::flush_routes()
{
    if (output_route_sink_ == nullptr) {
        return;
    }

    route_command command;
    while (route_queue_.pop_front(&command)) {
        output_route_sink_->submit(command);
    }
}

}  // namespace blusys::framework
