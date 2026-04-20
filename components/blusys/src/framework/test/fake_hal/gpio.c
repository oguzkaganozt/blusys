/* src/framework/test/fake_hal/gpio.c — in-memory GPIO for tests.
 *
 * Implements the full blusys_gpio_* ABI on an array of pin slots. Tests drive
 * pin levels via blusys_test_gpio_drive() and read back what product code
 * most recently wrote via blusys_test_gpio_last_write().
 *
 * Interrupt callbacks fire synchronously from the drive site — same thread
 * as the reducer. That matches the "one task owns state" invariant in the
 * threading contract (docs/internals/threading.md).
 */

#include "blusys/framework/test/fake_hal.h"
#include "blusys/hal/gpio.h"

#include <string.h>

#define FAKE_GPIO_MAX_PINS 64

typedef struct {
    bool                         input;
    bool                         output;
    blusys_gpio_pull_t           pull;
    blusys_gpio_interrupt_mode_t int_mode;
    blusys_gpio_callback_t       cb;
    void                        *cb_ctx;
    bool                         level_in;   /* what external agent drove */
    bool                         level_out;  /* what product code wrote */
} fake_pin_t;

static fake_pin_t g_pins[FAKE_GPIO_MAX_PINS];

static bool pin_ok(int pin) { return pin >= 0 && pin < FAKE_GPIO_MAX_PINS; }

blusys_err_t blusys_gpio_reset(int pin)
{
    if (!pin_ok(pin)) return BLUSYS_ERR_INVALID_ARG;
    memset(&g_pins[pin], 0, sizeof(g_pins[pin]));
    return BLUSYS_OK;
}

blusys_err_t blusys_gpio_set_input(int pin)
{
    if (!pin_ok(pin)) return BLUSYS_ERR_INVALID_ARG;
    g_pins[pin].input = true;
    g_pins[pin].output = false;
    return BLUSYS_OK;
}

blusys_err_t blusys_gpio_set_output(int pin)
{
    if (!pin_ok(pin)) return BLUSYS_ERR_INVALID_ARG;
    g_pins[pin].output = true;
    g_pins[pin].input  = false;
    return BLUSYS_OK;
}

blusys_err_t blusys_gpio_set_pull_mode(int pin, blusys_gpio_pull_t pull)
{
    if (!pin_ok(pin)) return BLUSYS_ERR_INVALID_ARG;
    g_pins[pin].pull = pull;
    return BLUSYS_OK;
}

blusys_err_t blusys_gpio_read(int pin, bool *out_level)
{
    if (!pin_ok(pin) || out_level == NULL) return BLUSYS_ERR_INVALID_ARG;
    *out_level = g_pins[pin].level_in;
    return BLUSYS_OK;
}

blusys_err_t blusys_gpio_write(int pin, bool level)
{
    if (!pin_ok(pin)) return BLUSYS_ERR_INVALID_ARG;
    g_pins[pin].level_out = level;
    return BLUSYS_OK;
}

blusys_err_t blusys_gpio_set_interrupt(int pin, blusys_gpio_interrupt_mode_t mode)
{
    if (!pin_ok(pin)) return BLUSYS_ERR_INVALID_ARG;
    g_pins[pin].int_mode = mode;
    return BLUSYS_OK;
}

blusys_err_t blusys_gpio_set_callback(int pin, blusys_gpio_callback_t cb, void *user_ctx)
{
    if (!pin_ok(pin)) return BLUSYS_ERR_INVALID_ARG;
    g_pins[pin].cb     = cb;
    g_pins[pin].cb_ctx = user_ctx;
    return BLUSYS_OK;
}

/* ── test inject API ──────────────────────────────────────────────────── */

static bool edge_matches(blusys_gpio_interrupt_mode_t mode, bool prev, bool now)
{
    switch (mode) {
    case BLUSYS_GPIO_INTERRUPT_RISING:     return !prev &&  now;
    case BLUSYS_GPIO_INTERRUPT_FALLING:    return  prev && !now;
    case BLUSYS_GPIO_INTERRUPT_ANY_EDGE:   return prev != now;
    case BLUSYS_GPIO_INTERRUPT_HIGH_LEVEL: return  now;
    case BLUSYS_GPIO_INTERRUPT_LOW_LEVEL:  return !now;
    case BLUSYS_GPIO_INTERRUPT_DISABLED:   break;
    }
    return false;
}

void blusys_test_gpio_drive(int pin, bool level)
{
    if (!pin_ok(pin)) return;
    bool prev = g_pins[pin].level_in;
    g_pins[pin].level_in = level;
    if (g_pins[pin].cb && edge_matches(g_pins[pin].int_mode, prev, level)) {
        g_pins[pin].cb(pin, g_pins[pin].cb_ctx);
    }
}

bool blusys_test_gpio_last_write(int pin)
{
    if (!pin_ok(pin)) return false;
    return g_pins[pin].level_out;
}

void blusys_test_gpio_reset_all(void)
{
    memset(g_pins, 0, sizeof(g_pins));
}
