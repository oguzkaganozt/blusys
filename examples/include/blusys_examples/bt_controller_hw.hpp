#pragma once

// Shared peripheral scaffolding for the Bluetooth-controller quickstarts
// (rotary encoder, status LED, battery ADC).  Radio stack and deep-sleep
// policy stay in each example — those diverge between BLE and Classic BT.

#include "blusys_examples/clamp_percent.hpp"
#include "blusys/blusys.hpp"


#ifdef ESP_PLATFORM
#endif

#include <cstdint>

namespace blusys_examples {

namespace detail { inline constexpr const char *kHwTag = "bt_ctrl_hw"; }

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

struct battery_config {
    int adc_pin  = -1;
    int mv_empty = 0;
    int mv_full  = 4200;
};

class battery_monitor {
public:
    void init(const battery_config &cfg) noexcept
    {
        cfg_ = cfg;
#ifdef ESP_PLATFORM
        if (cfg_.adc_pin < 0) {
            return;
        }
        blusys_err_t err = blusys_adc_open(cfg_.adc_pin,
                                           BLUSYS_ADC_ATTEN_DB_12,
                                           &adc_);
        if (err != BLUSYS_OK) {
            BLUSYS_LOGW(detail::kHwTag,
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
        pct = static_cast<int>(clamp_percent(pct));
        BLUSYS_LOGI(detail::kHwTag, "battery: %d mV -> %d%%", mv, pct);
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
};

#ifdef ESP_PLATFORM

struct encoder_config {
    int           clk_pin       = -1;
    int           dt_pin        = -1;
    int           sw_pin        = -1;
    std::uint32_t long_press_ms = 500;
};

enum class encoder_event : std::uint8_t {
    cw, ccw, press, release, long_press
};

// Maps a platform-neutral encoder event to an example-specific intent enum.
// Requires IntentEnum to expose: turn_cw, turn_ccw, button_press,
// button_release, button_long_press — the common vocabulary shared by the
// BLE and Classic BT controller reducers.
template <typename IntentEnum>
constexpr IntentEnum map_encoder_intent(encoder_event ev) noexcept
{
    switch (ev) {
    case encoder_event::cw:         return IntentEnum::turn_cw;
    case encoder_event::ccw:        return IntentEnum::turn_ccw;
    case encoder_event::press:      return IntentEnum::button_press;
    case encoder_event::release:    return IntentEnum::button_release;
    case encoder_event::long_press: return IntentEnum::button_long_press;
    }
    return IntentEnum::turn_cw;  // unreachable — enum class has 5 values
}

class encoder_input {
public:
    using handler_fn = void (*)(encoder_event ev);

    blusys_err_t open(const encoder_config &cfg, handler_fn handler) noexcept
    {
        handler_ = handler;

        blusys_encoder_config_t cc{};
        cc.clk_pin       = cfg.clk_pin;
        cc.dt_pin        = cfg.dt_pin;
        cc.sw_pin        = cfg.sw_pin;
        cc.long_press_ms = cfg.long_press_ms;

        blusys_err_t err = blusys_encoder_open(&cc, &enc_);
        if (err != BLUSYS_OK) {
            BLUSYS_LOGE(detail::kHwTag, "encoder_open failed: %d", static_cast<int>(err));
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
};

#endif  // ESP_PLATFORM

}  // namespace blusys_examples
