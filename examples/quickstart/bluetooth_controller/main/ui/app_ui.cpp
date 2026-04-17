#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "ui/app_ui.hpp"
#include "blusys/hal/log.h"

namespace bluetooth_controller::ui {

void on_init(blusys::app_ctx & /*ctx*/, blusys::app_services & /*svc*/, app_state & /*state*/)
{
    BLUSYS_LOGI("bluetooth_controller", "headless — no UI surface");
}

}  // namespace bluetooth_controller::ui

#endif  // BLUSYS_FRAMEWORK_HAS_UI
