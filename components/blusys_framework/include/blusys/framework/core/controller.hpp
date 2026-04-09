#pragma once

#include "blusys/error.h"
#include "blusys/framework/core/feedback.hpp"
#include "blusys/framework/core/intent.hpp"
#include "blusys/framework/core/router.hpp"

#include <cstdint>

namespace blusys::framework {

class controller {
public:
    virtual ~controller() = default;

    void bind_route_sink(route_sink *sink);
    void bind_feedback_bus(feedback_bus *bus);

    virtual blusys_err_t init();
    virtual void handle(const app_event &event) = 0;
    virtual void tick(std::uint32_t now_ms);
    virtual void deinit();

protected:
    bool submit_route(const route_command &command) const;
    void emit_feedback(const feedback_event &event) const;

    [[nodiscard]] route_sink *bound_route_sink() const;
    [[nodiscard]] feedback_bus *bound_feedback_bus() const;

private:
    route_sink   *route_sink_ = nullptr;
    feedback_bus *feedback_bus_ = nullptr;
};

}  // namespace blusys::framework
