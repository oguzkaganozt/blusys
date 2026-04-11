// interactive_archetype_reducer_smoke — host-only assertions using the same
// clamp_percent helper as the Phase 5 interactive archetype examples
// (`examples/include/blusys_examples/archetype_percent.hpp`).

#include "blusys_examples/archetype_percent.hpp"

#include <cstdint>
#include <cstdlib>

namespace {

bool run_tests()
{
    using blusys_examples::clamp_percent;

    // interactive_controller: level_delta respects hold (simulated).
    bool hold = false;
    std::int32_t level = 64;
    std::int32_t d = 10;
    if (!hold) {
        level = clamp_percent(level + d);
    }
    if (level != 74) {
        return false;
    }
    hold = true;
    d = -100;
    if (!hold) {
        level = clamp_percent(level + d);
    }
    if (level != 74) {
        return false;
    }

    // interactive_controller: clamp extremes
    if (clamp_percent(-5) != 0 || clamp_percent(200) != 100) {
        return false;
    }

    // interactive_panel: same helper for load_percent
    if (clamp_percent(-1) != 0 || clamp_percent(150) != 100) {
        return false;
    }

    return true;
}

}  // namespace

int main()
{
    return run_tests() ? EXIT_SUCCESS : EXIT_FAILURE;
}
