#pragma once

#include "blusys/framework/core/router.hpp"
#include "blusys/log.h"

namespace blusys::app::detail {

// Fallback route sink when the UI stack is compiled in but no screen router is used
// (headless-style builds with BLUSYS_FRAMEWORK_HAS_UI for tests).
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

}  // namespace blusys::app::detail
