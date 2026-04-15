// action_queue_drop_smoke — verifies app_ctx::action_queue_drop_count() tracks full queue.
// Build: blusys_framework_host + app_headless_platform (SDL timer only).

#include "blusys/framework/app/internal/app_runtime.hpp"
#include "blusys/hal/error.h"

#include <cstdlib>

namespace {

struct State {};

enum class Action { X };

void update(blusys::app_ctx &, State &, const Action &) {}

}  // namespace

int main()
{
    blusys::app_spec<State, Action> spec{
        .initial_state = {},
        .update        = update,
    };

    blusys::app_runtime<State, Action, 2> runtime(spec);
    if (runtime.init() != BLUSYS_OK) {
        return 1;
    }

    const Action a = Action::X;
    if (!runtime.ctx().dispatch(a)) {
        runtime.deinit();
        return 2;
    }
    if (!runtime.ctx().dispatch(a)) {
        runtime.deinit();
        return 3;
    }
    if (runtime.ctx().action_queue_drop_count() != 0U) {
        runtime.deinit();
        return 4;
    }
    if (runtime.ctx().dispatch(a)) {
        runtime.deinit();
        return 5;
    }
    if (runtime.ctx().action_queue_drop_count() != 1U) {
        runtime.deinit();
        return 6;
    }

    runtime.deinit();
    return 0;
}
