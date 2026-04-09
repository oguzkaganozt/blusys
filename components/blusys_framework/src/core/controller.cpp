#include "blusys/framework/core/controller.hpp"

namespace blusys::framework {

void controller::bind_route_sink(route_sink *sink)
{
    route_sink_ = sink;
}

void controller::bind_feedback_bus(feedback_bus *bus)
{
    feedback_bus_ = bus;
}

blusys_err_t controller::init()
{
    return BLUSYS_OK;
}

void controller::tick(std::uint32_t now_ms)
{
    (void)now_ms;
}

void controller::deinit()
{
}

bool controller::submit_route(const route_command &command) const
{
    if (route_sink_ == nullptr) {
        return false;
    }

    route_sink_->submit(command);
    return true;
}

void controller::emit_feedback(const feedback_event &event) const
{
    if (feedback_bus_ != nullptr) {
        feedback_bus_->emit(event);
    }
}

route_sink *controller::bound_route_sink() const
{
    return route_sink_;
}

feedback_bus *controller::bound_feedback_bus() const
{
    return feedback_bus_;
}

}  // namespace blusys::framework
