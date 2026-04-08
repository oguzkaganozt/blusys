# Enter Light And Deep Sleep With Timer Wakeup

## Problem Statement

You want to reduce power consumption by putting the chip to sleep between work intervals, waking after a fixed time.

## Prerequisites

- any supported board
- no external wiring required for timer wakeup

## Minimal Example (Light Sleep)

```c
#include <stdio.h>

#include "blusys/sleep.h"

void app_main(void)
{
    blusys_sleep_enable_timer_wakeup(5000000);   /* 5 s */
    blusys_sleep_enter_light();                  /* returns after wakeup */

    blusys_sleep_wakeup_t cause = blusys_sleep_get_wakeup_cause();
    if (cause == BLUSYS_SLEEP_WAKEUP_TIMER) {
        printf("woke from timer\n");
    }
}
```

## Minimal Example (Deep Sleep)

```c
blusys_sleep_enable_timer_wakeup(10000000);   /* 10 s */
blusys_sleep_enter_deep();                    /* resets on wakeup — does not return */
```

## APIs Used

- `blusys_sleep_enable_timer_wakeup()` sets a timer wakeup source in microseconds
- `blusys_sleep_enable_ulp_wakeup()` enables the ULP coprocessor as a deep-sleep wake source after `blusys_ulp_start()`
- `blusys_sleep_enter_light()` halts the CPU and most peripherals; returns after wakeup; RAM is retained
- `blusys_sleep_enter_deep()` powers down everything except the RTC domain; resets on wakeup; RAM is lost
- `blusys_sleep_get_wakeup_cause()` returns which source fired after waking from light sleep, or at the start of `app_main()` after deep sleep wakeup

## Expected Runtime Behavior

- light sleep: task execution resumes at the line after `blusys_sleep_enter_light()`
- deep sleep: `app_main()` is called from scratch; check `blusys_sleep_get_wakeup_cause()` at boot to distinguish deep-sleep wakeup from cold boot
- `BLUSYS_SLEEP_WAKEUP_UNDEFINED` is returned on the first cold boot

## Common Mistakes

- forgetting that deep sleep resets all RAM and peripherals — any state must be stored in NVS or RTC slow memory (`RTC_DATA_ATTR`) before entering
- enabling multiple wakeup sources but only checking one in the handler

## Example App

See `examples/sleep_basic/` for a runnable example.

Build and run it with the helper scripts or use the pattern shown in `guides/getting-started.md`.


## API Reference

For full type definitions and function signatures, see [Sleep API Reference](../modules/sleep.md).
