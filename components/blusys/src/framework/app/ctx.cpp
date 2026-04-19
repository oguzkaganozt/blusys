#include "blusys/framework/app/ctx.hpp"
#include "blusys/framework/app/internal/app_runtime.hpp"
#include "blusys/framework/capabilities/connectivity.hpp"

namespace blusys {

void app_ctx::emit_feedback(blusys::feedback_channel channel,
                            blusys::feedback_pattern pattern,
                            std::uint32_t value)
{
    if (feedback_bus_ != nullptr) {
        feedback_bus_->emit({
            .channel = channel,
            .pattern = pattern,
            .value   = value,
            .payload = nullptr,
        });
    }
}

blusys_err_t app_ctx::request_connectivity_reconnect()
{
    auto *conn = get<connectivity_capability>();
    if (conn == nullptr) {
        return BLUSYS_ERR_INVALID_STATE;
    }
    return conn->request_reconnect();
}

}  // namespace blusys
