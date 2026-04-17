#pragma once

// Shared hardware scaffolding for Bluetooth-controller style examples
// (rotary encoder + status LED + battery ADC).  Radio stack and deep-sleep
// policy stay in each example — those diverge between BLE and Classic BT.
//
// Host build: everything compiles to no-ops so example scaffolds stay
// portable.

#include "blusys/hal/error.h"
#include "blusys/hal/log.h"

#ifdef ESP_PLATFORM
#include "blusys/drivers/encoder.h"
#include "blusys/hal/adc.h"
#include "blusys/hal/gpio.h"
#endif

#include <cstdint>

namespace blusys_examples {

// Status LED ─────────────────────────────────────────────────────────────────
// Pin < 0 ⇒ disabled (init and set are no-ops).  active_low inverts the
// output polarity for LEDs wired cathode-to-GPIO.
struct status_led_config {
    int  pin        = -1;
    bool active_low = false;
};

class status_led {
public:
    void init(const status_led_config &cfg) noexcept
    {
        cfg_ = cfg;
#ifdef ESP_PLATFORM
        if (cfg_.pin < 0) return;
        blusys_gpio_reset(cfg_.pin);
        blusys_gpio_set_output(cfg_.pin);
        set(false);
#endif
    }

    void set(bool on) const noexcept
    {
#ifdef ESP_PLATFORM
        if (cfg_.pin < 0) return;
        blusys_gpio_write(cfg_.pin, cfg_.active_low ? !on : on);
#else
        (void)on;
#endif
    }

private:
    status_led_config cfg_{};
};

// Battery monitor ─────────────────────────────────────────────────────────────
// ADC-backed, linear mv → pct mapping clamped to [0, 100].  adc_pin < 0 ⇒
// disabled.  sample_pct() returns -1 on any failure.
struct battery_config {
    int adc_pin  = -1;
    int mv_empty = 0;
    int mv_full  = 4200;
};

class battery_monitor {
public:
    void init(const battery_config &cfg, const char *tag) noexcept
    {
        cfg_ = cfg;
        tag_ = (tag != nullptr) ? tag : "battery";
#ifdef ESP_PLATFORM
        if (cfg_.adc_pin < 0) {
            return;
        }
        blusys_err_t err = blusys_adc_open(cfg_.adc_pin,
                                           BLUSYS_ADC_ATTEN_DB_12,
                                           &adc_);
        if (err != BLUSYS_OK) {
            BLUSYS_LOGW(tag_,
                        "battery ADC open(pin=%d) failed: %d — disabled",
                        cfg_.adc_pin, static_cast<int>(err));
            adc_ = nullptr;
        }
#endif
    }

    [[nodiscard]] bool enabled() const noexcept
    {
#ifdef ESP_PLATFORM
        return adc_ != nullptr;
#else
        return false;
#endif
    }

    // Returns 0..100 on success, -1 on failure / disabled.
    int sample_pct() const noexcept
    {
#ifdef ESP_PLATFORM
        if (adc_ == nullptr) return -1;
        int mv = 0;
        if (blusys_adc_read_mv(adc_, &mv) != BLUSYS_OK) return -1;
        int range = cfg_.mv_full - cfg_.mv_empty;
        int pct   = (range > 0) ? (mv - cfg_.mv_empty) * 100 / range : 0;
        if (pct < 0)   pct = 0;
        if (pct > 100) pct = 100;
        BLUSYS_LOGI(tag_, "battery: %d mV -> %d%%", mv, pct);
        return pct;
#else
        return -1;
#endif
    }

private:
#ifdef ESP_PLATFORM
    blusys_adc_t  *adc_ = nullptr;
#endif
    battery_config cfg_{};
    const char    *tag_ = "battery";
};

// Encoder input (device-only; host build omits this class) ───────────────────
#ifdef ESP_PLATFORM

struct encoder_config {
    int           clk_pin       = -1;
    int           dt_pin        = -1;
    int           sw_pin        = -1;
    std::uint32_t long_press_ms = 500;
};

// Platform-neutral event enum so example reducers can map to their own
// intent type with a compact 5-case switch.
enum class encoder_event : std::uint8_t {
    cw, ccw, press, release, long_press
};

class encoder_input {
public:
    using handler_fn = void (*)(encoder_event ev);

    blusys_err_t open(const encoder_config &cfg, handler_fn handler,
                      const char *tag) noexcept
    {
        handler_ = handler;
        tag_     = (tag != nullptr) ? tag : "encoder";

        blusys_encoder_config_t cc{};
        cc.clk_pin       = cfg.clk_pin;
        cc.dt_pin        = cfg.dt_pin;
        cc.sw_pin        = cfg.sw_pin;
        cc.long_press_ms = cfg.long_press_ms;

        blusys_err_t err = blusys_encoder_open(&cc, &enc_);
        if (err != BLUSYS_OK) {
            BLUSYS_LOGE(tag_, "encoder_open failed: %d", static_cast<int>(err));
            return err;
        }
        return blusys_encoder_set_callback(enc_, &encoder_input::trampoline, this);
    }

private:
    static void trampoline(blusys_encoder_t * /*enc*/,
                           blusys_encoder_event_t ev,
                           int /*position*/,
                           void *user) noexcept
    {
        auto *self = static_cast<encoder_input *>(user);
        if (self == nullptr || self->handler_ == nullptr) {
            return;
        }
        switch (ev) {
        case BLUSYS_ENCODER_EVENT_CW:         self->handler_(encoder_event::cw);         break;
        case BLUSYS_ENCODER_EVENT_CCW:        self->handler_(encoder_event::ccw);        break;
        case BLUSYS_ENCODER_EVENT_PRESS:      self->handler_(encoder_event::press);      break;
        case BLUSYS_ENCODER_EVENT_RELEASE:    self->handler_(encoder_event::release);    break;
        case BLUSYS_ENCODER_EVENT_LONG_PRESS: self->handler_(encoder_event::long_press); break;
        default: break;
        }
    }

    blusys_encoder_t *enc_     = nullptr;
    handler_fn        handler_ = nullptr;
    const char       *tag_     = "encoder";
};

#endif  // ESP_PLATFORM

}  // namespace blusys_examples
