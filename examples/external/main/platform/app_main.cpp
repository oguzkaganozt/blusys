#include <blusys/blusys.hpp>

#include <cstdint>

namespace external_lockdown {

struct app_state {
    std::uint32_t tick_count = 0;
    std::uint32_t action_count = 0;
};

enum class action_tag : std::uint8_t {
    ticked,
};

struct action {
    action_tag tag = action_tag::ticked;
};

void update(blusys::app_ctx &ctx, app_state &state, const action &event)
{
    (void)ctx;
    if (event.tag == action_tag::ticked) {
        ++state.action_count;
    }
}

void on_tick(blusys::app_ctx &ctx, blusys::app_fx &fx, app_state &state, std::uint32_t now_ms)
{
    (void)fx;
    (void)now_ms;
    ++state.tick_count;
    ctx.dispatch(action{});
}

}  // namespace external_lockdown

const blusys::app_spec<external_lockdown::app_state, external_lockdown::action> spec{
    .initial_state = {},
    .update = external_lockdown::update,
    .on_tick = external_lockdown::on_tick,
    .tick_period_ms = 1000,
};

BLUSYS_APP_HEADLESS(spec)
