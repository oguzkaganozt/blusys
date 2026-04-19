#pragma once

#include "blusys/framework/events/router.hpp"
#include "blusys/hal/log.h"

namespace blusys::detail {

// Fallback route sink when the UI stack is compiled in but no screen router is used
// (headless-style builds with BLUSYS_FRAMEWORK_HAS_UI for tests).
class default_route_sink final : public blusys::route_sink {
public:
    void submit(const blusys::route_command &command) override
    {
        BLUSYS_LOGI("blusys_app",
                    "route: %s id=%lu",
                    blusys::route_command_type_name(command.type),
                    static_cast<unsigned long>(command.id));
    }
};

}  // namespace blusys::detail
