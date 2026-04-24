# Button

GPIO button abstraction with software debounce and long-press detection.

## At a glance

- one callback per handle
- events fire from the esp_timer task
- `read()` returns raw level, not debounced state

## Quick example

```c
static void on_button(blusys_button_t *btn, blusys_button_event_t event, void *ctx)
{
    (void)btn; (void)ctx;
    if (event == BLUSYS_BUTTON_EVENT_PRESS) printf("pressed\n");
}

blusys_button_t *btn;
blusys_button_config_t cfg = {
    .pin = 2,
    .pull_mode = BLUSYS_GPIO_PULL_UP,
    .active_low = true,
};
blusys_button_open(&cfg, &btn);
blusys_button_set_callback(btn, on_button, NULL);
```

## Common mistakes

- leaving the pin floating
- using the wrong active level
- closing from inside the callback
- treating `read()` as debounced

## Target support

**ESP32, ESP32-C3, ESP32-S3** — all supported.

## Callback context

- callbacks run in the esp_timer task
- it is safe to call most Blusys APIs from the callback

## Thread safety

- different handles may be used concurrently
- callback changes wait for in-flight callbacks to finish

## Limitations

- one callback per handle
- long-press fires once per press

## Example app

`examples/reference/hal`
