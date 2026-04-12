// framework_encoder_basic — real lv_indev_t encoder traversal end-to-end.
//
// Fourth scenario in the framework example set:
// a real LVGL encoder input device wired into the framework's encoder
// helpers, driving focus traversal and activation across the V1 widget
// kit. The other three framework examples cover spine in isolation
// (`framework_core_basic`), layout primitives + interactive widgets
// (`framework_ui_basic`), and full spine + widgets with simulated input
// (`framework_app_basic`); this one is the only example that runs a
// genuine `lv_indev_t` of type `LV_INDEV_TYPE_ENCODER` against the
// `create_encoder_group` + `auto_focus_screen` helpers.
//
// The example bridges three layers:
//
//   1. blusys_encoder driver — hardware quadrature encoder with optional
//      press button. PCNT-backed on ESP32 / ESP32-S3, GPIO-ISR fallback
//      on ESP32-C3. Reports CW/CCW/PRESS/RELEASE events via callback.
//
//   2. LVGL `lv_indev_t` of type `LV_INDEV_TYPE_ENCODER` — receives the
//      encoder delta and button state via a read callback that returns
//      and clears whatever the driver callback has accumulated since
//      the previous poll.
//
//   3. `blusys::ui::create_encoder_group` + `auto_focus_screen` —
//      builds an LVGL focus group, walks the screen, and adds every
//      focusable widget. `lv_indev_set_group(indev, group)` wires the
//      indev to the group, after which encoder rotation moves focus
//      and a button press activates the focused widget.
//
// Headless display: this example does not initialise an LCD. It
// registers a stub `lv_display_t` whose flush callback is a no-op,
// which is the minimum LVGL setup required for `lv_screen_load`,
// `lv_indev_create`, and `lv_timer_handler` to work. This keeps the
// example portable across all three targets and runnable in CI without
// any display hardware. On real hardware, replace the stub display
// with a `blusys_lcd_open` + `blusys_ui_open` chain (as in
// `examples/ui_basic/`) to see the focus highlight on screen.
//
// What you should see in the log when running on hardware with a real
// encoder wired to the configured pins (CONFIG_BLUSYS_FRAMEWORK_ENCODER_BASIC_*):
//
//   - "encoder open: clk=N dt=N sw=N"
//   - "encoder CW (position=...)" each time the shaft rotates clockwise
//   - "encoder CCW (position=...)" each time it rotates counter-clockwise
//   - "encoder PRESS" / "encoder RELEASE" when the shaft button is pressed
//   - "button A pressed" / "button B pressed" when focus lands on a button
//     and the shaft is pressed (this is the indev → group → widget click
//     pipeline doing its job)
//   - "toggle changed" / slider updates when focus lands on those widgets
//     and the shaft is pressed/rotated

#include "blusys/framework/framework.hpp"
#include "blusys/framework/ui/widgets.hpp"
#include "blusys/app/theme_presets.hpp"
#include "blusys/log.h"

#include "blusys/drivers/input/encoder.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "sdkconfig.h"

namespace {

constexpr const char *kTag = "framework_encoder_basic";

// ---- stub display -----------------------------------------------------------
//
// LVGL refuses to load a screen or process indev events without at least
// one display registered. We register a tiny in-memory framebuffer with
// a no-op flush callback so the rest of the pipeline can run.

constexpr int32_t kStubWidth    = 160;
constexpr int32_t kStubHeight   = 80;
constexpr size_t  kStubBufLines = 10;

lv_color_t g_stub_buf[kStubWidth * kStubBufLines];

void stub_flush_cb(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map)
{
    (void)area;
    (void)px_map;
    lv_display_flush_ready(disp);
}

// ---- encoder → indev bridge -------------------------------------------------
//
// The encoder driver is callback-driven (CW/CCW/PRESS/RELEASE) but
// LVGL's indev API is poll-driven (read callback returns the current
// state). This struct is the trivial state buffer that turns one into
// the other. The encoder callback writes to it; the indev read callback
// reads and clears the rotation accumulator.

struct encoder_state {
    int32_t          pending_diff = 0;
    lv_indev_state_t button_state = LV_INDEV_STATE_RELEASED;
};

encoder_state g_encoder_state{};

void on_encoder_event(blusys_encoder_t *enc, blusys_encoder_event_t event,
                      int position, void *user_ctx)
{
    (void)enc;
    (void)user_ctx;

    switch (event) {
    case BLUSYS_ENCODER_EVENT_CW:
        g_encoder_state.pending_diff += 1;
        BLUSYS_LOGI(kTag, "encoder CW (position=%d)", position);
        break;
    case BLUSYS_ENCODER_EVENT_CCW:
        g_encoder_state.pending_diff -= 1;
        BLUSYS_LOGI(kTag, "encoder CCW (position=%d)", position);
        break;
    case BLUSYS_ENCODER_EVENT_PRESS:
        g_encoder_state.button_state = LV_INDEV_STATE_PRESSED;
        BLUSYS_LOGI(kTag, "encoder PRESS");
        break;
    case BLUSYS_ENCODER_EVENT_RELEASE:
        g_encoder_state.button_state = LV_INDEV_STATE_RELEASED;
        BLUSYS_LOGI(kTag, "encoder RELEASE");
        break;
    case BLUSYS_ENCODER_EVENT_LONG_PRESS:
        BLUSYS_LOGI(kTag, "encoder LONG_PRESS (no semantic mapping in this example)");
        break;
    }
}

void encoder_indev_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    data->enc_diff               = g_encoder_state.pending_diff;
    data->state                  = g_encoder_state.button_state;
    g_encoder_state.pending_diff = 0;
}

// ---- LVGL tick + timer pump task --------------------------------------------
//
// LVGL needs both a tick source (so it knows how much time has passed)
// and a periodic call to `lv_timer_handler` (which runs animations,
// drives indev polling, and dispatches widget events). A FreeRTOS task
// running every 10 ms is the simplest way to drive both from a single
// thread; real products typically use `esp_timer` for tick and a
// dedicated task for the handler.

constexpr TickType_t kPumpPeriodTicks = pdMS_TO_TICKS(10);

void lvgl_pump_task(void *arg)
{
    (void)arg;
    while (true) {
        lv_tick_inc(10);
        lv_timer_handler();
        vTaskDelay(kPumpPeriodTicks);
    }
}

// ---- click handlers for the focusable widgets -------------------------------

int  g_button_a_count = 0;
int  g_button_b_count = 0;
bool g_toggle_state   = false;

}  // namespace

extern "C" void app_main(void)
{
    lv_init();

    // ---- stub display so screen + indev pipeline have something to attach to

    lv_display_t *display = lv_display_create(kStubWidth, kStubHeight);
    lv_display_set_buffers(display, g_stub_buf, nullptr,
                           sizeof(g_stub_buf),
                           LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, stub_flush_cb);

    // ---- theme + screen layout ----------------------------------------------

    blusys::ui::set_theme(blusys::app::presets::expressive_dark());

    lv_obj_t *screen = blusys::ui::screen_create({});
    lv_obj_t *col = blusys::ui::col_create(screen, {
        .gap     = blusys::ui::theme().spacing_md,
        .padding = blusys::ui::theme().spacing_lg,
    });

    blusys::ui::label_create(col, {
        .text = "Encoder traversal",
        .font = blusys::ui::theme().font_title,
    });
    blusys::ui::divider_create(col, {});

    lv_obj_t *button_a = blusys::ui::button_create(col, {
        .label    = "Button A",
        .variant  = blusys::ui::button_variant::primary,
        .on_press = +[](void *user_data) {
            (void)user_data;
            ++g_button_a_count;
            BLUSYS_LOGI(kTag, "button A pressed (count=%d)", g_button_a_count);
        },
        .user_data = nullptr,
        .disabled  = false,
    });

    lv_obj_t *button_b = blusys::ui::button_create(col, {
        .label    = "Button B",
        .variant  = blusys::ui::button_variant::secondary,
        .on_press = +[](void *user_data) {
            (void)user_data;
            ++g_button_b_count;
            BLUSYS_LOGI(kTag, "button B pressed (count=%d)", g_button_b_count);
        },
        .user_data = nullptr,
        .disabled  = false,
    });

    lv_obj_t *toggle = blusys::ui::toggle_create(col, {
        .initial   = false,
        .on_change = +[](bool new_state, void *user_data) {
            (void)user_data;
            g_toggle_state = new_state;
            BLUSYS_LOGI(kTag, "toggle changed (state=%s)", new_state ? "on" : "off");
        },
        .user_data = nullptr,
        .disabled  = false,
    });

    lv_obj_t *slider = blusys::ui::slider_create(col, {
        .min       = 0,
        .max       = 100,
        .initial   = 25,
        .on_change = nullptr,
        .user_data = nullptr,
        .disabled  = false,
    });

    lv_screen_load(screen);

    // ---- encoder focus group + auto-focus walk ------------------------------

    lv_group_t *encoder_group = blusys::ui::create_encoder_group();
    blusys::ui::auto_focus_screen(screen, encoder_group);

    // ---- real lv_indev_t encoder bound to the group -------------------------

    lv_indev_t *indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_ENCODER);
    lv_indev_set_read_cb(indev, encoder_indev_read_cb);
    lv_indev_set_group(indev, encoder_group);

    // ---- open the hardware encoder driver -----------------------------------
    //
    // Pin assignments come from menuconfig. If the encoder cannot be
    // opened (no hardware, GPIO conflict, target with no PCNT and no
    // ISR fallback) the example still keeps running with the indev /
    // group / screen wired up — useful for the CI build gate where
    // there is no hardware to connect to.

    blusys_encoder_t              *encoder = nullptr;
    const blusys_encoder_config_t  enc_cfg = {
        .clk_pin          = CONFIG_BLUSYS_FRAMEWORK_ENCODER_BASIC_CLK_PIN,
        .dt_pin           = CONFIG_BLUSYS_FRAMEWORK_ENCODER_BASIC_DT_PIN,
        .sw_pin           = CONFIG_BLUSYS_FRAMEWORK_ENCODER_BASIC_SW_PIN,
        .glitch_filter_ns = 1000,
        .debounce_ms      = 50,
        .long_press_ms    = 1000,
        .steps_per_detent = 0,
    };
    const blusys_err_t enc_err = blusys_encoder_open(&enc_cfg, &encoder);
    if (enc_err != BLUSYS_OK) {
        BLUSYS_LOGW(kTag,
                    "encoder open failed (%s) — example will run with no input. "
                    "The lv_indev_t and group are still wired and verifiable.",
                    blusys_err_string(enc_err));
    } else {
        blusys_encoder_set_callback(encoder, on_encoder_event, nullptr);
        BLUSYS_LOGI(kTag,
                    "encoder open: clk=%d dt=%d sw=%d",
                    enc_cfg.clk_pin, enc_cfg.dt_pin, enc_cfg.sw_pin);
    }

    BLUSYS_LOGI(kTag,
                "wired: screen=%p button_a=%p button_b=%p toggle=%p slider=%p group=%p indev=%p",
                static_cast<void *>(screen),
                static_cast<void *>(button_a),
                static_cast<void *>(button_b),
                static_cast<void *>(toggle),
                static_cast<void *>(slider),
                static_cast<void *>(encoder_group),
                static_cast<void *>(indev));

    // ---- start the LVGL pump ------------------------------------------------

    xTaskCreate(lvgl_pump_task, "lvgl_pump", 4096, nullptr, 5, nullptr);
}
