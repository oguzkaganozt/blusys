/* blusys/framework/test/harness.hpp — headless reducer harness for tests.
 *
 * Wraps app_runtime<State, Action> with a manually-advanced clock so tests
 * can drive the reducer deterministically: no sleep, no SDL, no real timer.
 * The harness is the canonical host iteration loop described in P0c — any
 * test writing a blusys app spec uses it instead of BLUSYS_APP_MAIN_HEADLESS.
 *
 * Typical shape:
 *
 *   blusys::test::test_harness<MyState, MyAction> h(spec);
 *   BLUSYS_TEST_REQUIRE(h.init() == BLUSYS_OK);
 *   h.post_action(increment{});
 *   h.step();                       // advances by spec.tick_period_ms
 *   BLUSYS_TEST_REQUIRE(h.state().count == 1);
 *   h.deinit();
 *
 * The harness intentionally does not sleep or yield. Subsystems that need to
 * observe elapsed time must read it from the harness (via `now_ms()`) rather
 * than calling into the platform clock.
 */

#ifndef BLUSYS_FRAMEWORK_TEST_HARNESS_HPP
#define BLUSYS_FRAMEWORK_TEST_HARNESS_HPP

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "blusys/framework/app/internal/app_runtime.hpp"
#include "blusys/framework/app/spec.hpp"
#include "blusys/framework/test/fake_clock.hpp"
#include "blusys/hal/error.h"

namespace blusys::test {

template <typename State, typename Action>
class test_harness {
public:
    using spec_type = blusys::app_spec<State, Action>;

    explicit test_harness(const spec_type &spec, std::uint32_t start_ms = 0)
        : runtime_(spec), spec_(spec), clock_(start_ms)
    {
    }

    blusys_err_t init() { return runtime_.init(); }
    void         deinit() { runtime_.deinit(); }

    /* Advance one tick (spec.tick_period_ms) and dispatch everything queued. */
    void step() { step_for(spec_.tick_period_ms); }

    /* Advance by `ms` in one hop; callers that want multiple ticks should
     * call step() in a loop rather than passing a large value — this mirrors
     * the real runtime, which runs the reducer once per tick. */
    void step_for(std::uint32_t ms)
    {
        clock_.advance(ms);
        runtime_.step(clock_.now_ms());
    }

    /* Run `n` ticks of `spec.tick_period_ms` each. Useful when a capability's
     * poll() needs several iterations before emitting an event. */
    void run_ticks(std::size_t n)
    {
        for (std::size_t i = 0; i < n; ++i) {
            step();
        }
    }

    bool                post_action(const Action &action) { return runtime_.post_action(action); }
    void                post_intent(blusys::intent i,
                                    std::uint32_t  source_id = 0,
                                    const void    *payload = nullptr)
    {
        runtime_.post_intent(i, source_id, payload);
    }

    [[nodiscard]] const State &state() const { return runtime_.state(); }
    [[nodiscard]]       State &state()       { return runtime_.state(); }

    [[nodiscard]] std::uint32_t now_ms() const { return clock_.now_ms(); }
    [[nodiscard]] fake_clock   &clock()        { return clock_; }

    [[nodiscard]] blusys::runtime &framework_runtime() { return runtime_.framework_runtime(); }

private:
    blusys::app_runtime<State, Action> runtime_;
    spec_type                          spec_;
    fake_clock                         clock_;
};

/* Minimal assertion macro so tests don't pull in an assertion framework.
 * Prints a one-line failure via the observe log path if a condition fails
 * and returns from the current function with exit code 1. */
#define BLUSYS_TEST_REQUIRE(expr)                                                \
    do {                                                                         \
        if (!(expr)) {                                                           \
            std::fprintf(stderr, "[FAIL] %s:%d  %s\n", __FILE__, __LINE__, #expr); \
            std::exit(1);                                                        \
        }                                                                        \
    } while (0)

}  // namespace blusys::test

#endif  // BLUSYS_FRAMEWORK_TEST_HARNESS_HPP
