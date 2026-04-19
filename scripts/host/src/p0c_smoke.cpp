/* p0c_smoke — end-to-end smoke for the P0c test harness.
 *
 * Exercises, in one executable, everything P0c delivers:
 *
 *   1. test_harness<State, Action>   — init / step / post_action / deinit.
 *   2. fake HAL for gpio, timer, uart — drive a pin, advance virtual time,
 *      push bytes into a UART, drain transmitted bytes.
 *   3. three canonical capability mocks (storage, diagnostics, telemetry) —
 *      lifecycle counters tick as expected.
 *   4. integration-event pipeline — a mock emits event id 0x0200, the
 *      framework maps it to capability_event_tag::storage_spiffs_mounted,
 *      on_event routes it to an Action, the reducer mutates state.
 *
 * If any assertion fails the harness prints [FAIL] file:line expr and exits
 * non-zero. Success prints "p0c_smoke: all asserts passed" and returns 0.
 *
 * Link list is intentionally minimal — no libmosquitto, no SDL2, no LVGL.
 * The plan calls this the fast host iteration loop; proving a test can link
 * without those is the point.
 */

#include "blusys/framework/app/internal/app_runtime.hpp"
#include "blusys/framework/app/spec.hpp"
#include "blusys/framework/capabilities/event.hpp"
#include "blusys/framework/capabilities/list.hpp"
#include "blusys/framework/test/test.hpp"
#include "blusys/hal/error.h"
#include "blusys/hal/gpio.h"
#include "blusys/hal/timer.h"
#include "blusys/hal/uart.h"

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <optional>

namespace {

/* ── app spec ────────────────────────────────────────────────────────── */

enum ActionTag : std::uint8_t {
    ACTION_NONE,
    ACTION_INCREMENT,
    ACTION_CAP_EVENT,
};

struct Action {
    ActionTag                tag = ACTION_NONE;
    blusys::capability_event cap_event{};
};

struct State {
    int  counter      = 0;
    bool storage_seen = false;
};

void reducer(blusys::app_ctx &, State &state, const Action &action)
{
    switch (action.tag) {
    case ACTION_INCREMENT:
        ++state.counter;
        break;
    case ACTION_CAP_EVENT:
        if (action.cap_event.tag == blusys::capability_event_tag::storage_spiffs_mounted) {
            state.storage_seen = true;
        }
        break;
    case ACTION_NONE:
        break;
    }
}

std::optional<Action> on_event(blusys::event event, State &state)
{
    (void)state;
    if (event.source != blusys::event_source::integration) {
        return std::nullopt;
    }

    const auto *ce = blusys::event_capability(event);
    if (ce == nullptr) {
        return std::nullopt;
    }
    return Action{.tag = ACTION_CAP_EVENT, .cap_event = *ce};
}

/* ── timer callback (stateless lambda can't capture) ─────────────────── */

static int g_timer_hits = 0;

bool on_timer(blusys_timer_t *, void *)
{
    ++g_timer_hits;
    return false;
}

/* ── uart rx callback ────────────────────────────────────────────────── */

static std::uint8_t g_rx_buf[16]{};
static std::size_t  g_rx_len = 0;

void on_uart_rx(blusys_uart_t *, const void *data, std::size_t size, void *)
{
    std::size_t n = size < sizeof(g_rx_buf) ? size : sizeof(g_rx_buf);
    std::memcpy(g_rx_buf, data, n);
    g_rx_len = n;
}

}  // namespace

int main()
{
    using namespace blusys::test;

    /* ── set up mocks + capability list ──────────────────────────────── */
    storage_mock     storage;
    diagnostics_mock diagnostics;
    telemetry_mock   telemetry;

    blusys::capability_list_storage caps{&storage, &diagnostics, &telemetry};

    blusys::app_spec<State, Action> spec{
        .initial_state = {},
        .update        = reducer,
        .on_event      = on_event,
        .capabilities  = &caps,
    };

    test_harness<State, Action> harness(spec);

    /* ── lifecycle: init ─────────────────────────────────────────────── */
    BLUSYS_TEST_REQUIRE(harness.init() == BLUSYS_OK);
    BLUSYS_TEST_REQUIRE(storage.start_calls     == 1);
    BLUSYS_TEST_REQUIRE(diagnostics.start_calls == 1);
    BLUSYS_TEST_REQUIRE(telemetry.start_calls   == 1);
    BLUSYS_TEST_REQUIRE(storage.status.capability_ready);

    /* ── reducer: post an action, confirm state mutates ──────────────── */
    BLUSYS_TEST_REQUIRE(harness.post_action(Action{.tag = ACTION_INCREMENT}));
    harness.step();
    BLUSYS_TEST_REQUIRE(harness.state().counter == 1);
    BLUSYS_TEST_REQUIRE(storage.poll_calls >= 1);

    /* ── fake GPIO ───────────────────────────────────────────────────── */
    BLUSYS_TEST_REQUIRE(blusys_gpio_set_output(5) == BLUSYS_OK);
    BLUSYS_TEST_REQUIRE(blusys_gpio_write(5, true) == BLUSYS_OK);
    BLUSYS_TEST_REQUIRE(blusys_test_gpio_last_write(5) == true);

    BLUSYS_TEST_REQUIRE(blusys_gpio_set_input(7) == BLUSYS_OK);
    blusys_test_gpio_drive(7, true);
    bool level = false;
    BLUSYS_TEST_REQUIRE(blusys_gpio_read(7, &level) == BLUSYS_OK);
    BLUSYS_TEST_REQUIRE(level);

    /* ── fake Timer ──────────────────────────────────────────────────── */
    blusys_timer_t *t = nullptr;
    BLUSYS_TEST_REQUIRE(blusys_timer_open(1000, true, &t) == BLUSYS_OK);
    BLUSYS_TEST_REQUIRE(blusys_timer_set_callback(t, on_timer, nullptr) == BLUSYS_OK);
    BLUSYS_TEST_REQUIRE(blusys_timer_start(t) == BLUSYS_OK);

    std::size_t hits = blusys_test_timer_advance_us(5500);
    BLUSYS_TEST_REQUIRE(hits == 5);
    BLUSYS_TEST_REQUIRE(g_timer_hits == 5);
    BLUSYS_TEST_REQUIRE(blusys_timer_close(t) == BLUSYS_OK);

    /* ── fake UART ───────────────────────────────────────────────────── */
    blusys_uart_t *u = nullptr;
    BLUSYS_TEST_REQUIRE(blusys_uart_open(0, 1, 2, 115200, &u) == BLUSYS_OK);
    BLUSYS_TEST_REQUIRE(blusys_uart_set_rx_callback(u, on_uart_rx, nullptr) == BLUSYS_OK);

    const char *rx_payload = "hi";
    blusys_test_uart_push_rx(0, rx_payload, 2);
    BLUSYS_TEST_REQUIRE(g_rx_len == 2);
    BLUSYS_TEST_REQUIRE(g_rx_buf[0] == 'h' && g_rx_buf[1] == 'i');

    BLUSYS_TEST_REQUIRE(blusys_uart_write(u, "OK\n", 3, 0) == BLUSYS_OK);
    std::uint8_t tx[16]{};
    std::size_t  n_tx = blusys_test_uart_take_tx(0, tx, sizeof(tx));
    BLUSYS_TEST_REQUIRE(n_tx == 3);
    BLUSYS_TEST_REQUIRE(tx[0] == 'O' && tx[1] == 'K' && tx[2] == '\n');
    BLUSYS_TEST_REQUIRE(blusys_uart_close(u) == BLUSYS_OK);

    /* ── integration-event pipeline ──────────────────────────────────── *
     * storage_mock emits raw id 0x0200 → framework maps to
     * storage_spiffs_mounted → on_event returns Action::CAP_EVENT →
     * reducer flips state.storage_seen.                                  */
    storage.emit(0x0200u);
    harness.step();
    BLUSYS_TEST_REQUIRE(harness.state().storage_seen);

    /* ── lifecycle: deinit ───────────────────────────────────────────── */
    harness.deinit();
    BLUSYS_TEST_REQUIRE(storage.stop_calls     == 1);
    BLUSYS_TEST_REQUIRE(diagnostics.stop_calls == 1);
    BLUSYS_TEST_REQUIRE(telemetry.stop_calls   == 1);

    std::fprintf(stdout, "p0c_smoke: all asserts passed\n");
    return 0;
}
