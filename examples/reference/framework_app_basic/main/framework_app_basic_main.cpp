// framework_app_basic — end-to-end Phase 6 validation.
//
// Closes the Phase 6 "done when" criterion: a sample app that runs
// the full chain from input → intent → controller → route command →
// screen update → feedback. Encoder hardware is intentionally not
// involved (no QEMU encoder driver) — the example uses three
// `bu_button` widgets as the input source. Each button posts a
// semantic intent to the runtime; the controller reacts by either
// mutating the slider directly (increment/decrement) or submitting a
// route command (confirm → show_overlay) which a custom `route_sink`
// then applies to the actual overlay widget.
//
// This is the smallest possible exercise of the full spine: every
// connector in the framework — `runtime`, `controller`, `route_sink`,
// `feedback_bus`, `feedback_sink`, the widget kit, and the encoder
// helpers — is wired up and exercised in this one file.

#include "blusys/framework/framework.hpp"
#include "blusys/framework/ui/widgets.hpp"
#include "blusys/log.h"

namespace {

constexpr const char *kTag = "framework_app_basic";

constexpr int32_t kSliderMin   = 0;
constexpr int32_t kSliderMax   = 100;
constexpr int32_t kSliderStep  = 5;
constexpr int32_t kSliderInit  = 50;

int32_t clamp_slider(int32_t value)
{
    if (value < kSliderMin) return kSliderMin;
    if (value > kSliderMax) return kSliderMax;
    return value;
}

// ---- shared app state -------------------------------------------------------

struct app_state {
    lv_obj_t *slider = nullptr;
    lv_obj_t *toast  = nullptr;
};

// ---- feedback sink ----------------------------------------------------------

class logging_feedback_sink final : public blusys::framework::feedback_sink {
public:
    bool supports(blusys::framework::feedback_channel) const override
    {
        return true;
    }

    void emit(const blusys::framework::feedback_event &event) override
    {
        BLUSYS_LOGI(kTag,
                    "feedback: channel=%s pattern=%s value=%lu",
                    blusys::framework::feedback_channel_name(event.channel),
                    blusys::framework::feedback_pattern_name(event.pattern),
                    static_cast<unsigned long>(event.value));
    }
};

// ---- route sink: applies route commands to real widgets ---------------------

class ui_route_sink final : public blusys::framework::route_sink {
public:
    void bind(app_state *state) { state_ = state; }

    void submit(const blusys::framework::route_command &command) override
    {
        BLUSYS_LOGI(kTag,
                    "route: %s id=%lu",
                    blusys::framework::route_command_type_name(command.type),
                    static_cast<unsigned long>(command.id));

        if (state_ == nullptr) {
            return;
        }

        switch (command.type) {
        case blusys::framework::route_command_type::show_overlay:
            blusys::ui::overlay_show(state_->toast);
            break;
        case blusys::framework::route_command_type::hide_overlay:
            blusys::ui::overlay_hide(state_->toast);
            break;
        default:
            // set_root / push / replace / pop are not exercised in this demo;
            // a real product's route sink would dispatch them to a screen
            // registry. Logging them above is sufficient for V1 validation.
            break;
        }
    }

private:
    app_state *state_ = nullptr;
};

// ---- controller: handles intents, mutates slider, emits routes/feedback -----

class app_controller final : public blusys::framework::controller {
public:
    void bind(app_state *state) { state_ = state; }

    blusys_err_t init() override
    {
        emit_feedback({
            .channel = blusys::framework::feedback_channel::visual,
            .pattern = blusys::framework::feedback_pattern::focus,
            .value   = 1,
            .payload = nullptr,
        });
        return BLUSYS_OK;
    }

    void handle(const blusys::framework::app_event &event) override
    {
        if (event.kind != blusys::framework::app_event_kind::intent) {
            return;
        }
        if (state_ == nullptr || state_->slider == nullptr) {
            return;
        }

        switch (blusys::framework::app_event_intent(event)) {
        case blusys::framework::intent::increment: {
            const int32_t cur = blusys::ui::slider_get_value(state_->slider);
            blusys::ui::slider_set_value(state_->slider, clamp_slider(cur + kSliderStep));
            emit_click_feedback();
            break;
        }
        case blusys::framework::intent::decrement: {
            const int32_t cur = blusys::ui::slider_get_value(state_->slider);
            blusys::ui::slider_set_value(state_->slider, clamp_slider(cur - kSliderStep));
            emit_click_feedback();
            break;
        }
        case blusys::framework::intent::confirm: {
            // Submit a route command and let the ui_route_sink apply it.
            // This is the path that exercises the runtime's route flush
            // pipeline end-to-end.
            submit_route(blusys::framework::route::show_overlay(1));
            emit_feedback({
                .channel = blusys::framework::feedback_channel::audio,
                .pattern = blusys::framework::feedback_pattern::confirm,
                .value   = 1,
                .payload = nullptr,
            });
            break;
        }
        default:
            break;
        }
    }

private:
    void emit_click_feedback() const
    {
        emit_feedback({
            .channel = blusys::framework::feedback_channel::haptic,
            .pattern = blusys::framework::feedback_pattern::click,
            .value   = 1,
            .payload = nullptr,
        });
    }

    app_state *state_ = nullptr;
};

// ---- module-level singletons ------------------------------------------------

app_state                 g_state{};
app_controller            g_controller{};
ui_route_sink             g_route_sink{};
logging_feedback_sink     g_feedback_sink{};
blusys::framework::runtime g_runtime{};

// Button on_press lambdas need a stable runtime pointer in user_data.
blusys::framework::runtime *g_runtime_ptr = &g_runtime;

}  // namespace

extern "C" void app_main(void)
{
    blusys::framework::init();
    lv_init();

    blusys::ui::set_theme({
        .color_primary    = lv_color_hex(0x2A62FF),
        .color_surface    = lv_color_hex(0x11141D),
        .color_on_primary = lv_color_hex(0xF7F9FF),
        .color_accent     = lv_color_hex(0x69D6C8),
        .color_disabled   = lv_color_hex(0x667089),
        .spacing_sm  = 8,
        .spacing_md  = 12,
        .spacing_lg  = 20,
        .radius_card = 14,
        .radius_button = 999,
        .font_body   = LV_FONT_DEFAULT,
        .font_title  = LV_FONT_DEFAULT,
        .font_mono   = LV_FONT_DEFAULT,
    });

    // ---- screen layout ------------------------------------------------------

    lv_obj_t *screen = blusys::ui::screen_create({});
    lv_obj_t *col = blusys::ui::col_create(screen, {
        .gap     = blusys::ui::theme().spacing_md,
        .padding = blusys::ui::theme().spacing_lg,
    });
    blusys::ui::label_create(col, {
        .text = "Framework App",
        .font = blusys::ui::theme().font_title,
    });
    blusys::ui::divider_create(col, {});
    blusys::ui::label_create(col, {
        .text = "Volume",
        .font = blusys::ui::theme().font_body,
    });

    g_state.slider = blusys::ui::slider_create(col, {
        .min       = kSliderMin,
        .max       = kSliderMax,
        .initial   = kSliderInit,
        .on_change = nullptr,
        .user_data = nullptr,
        .disabled  = false,
    });

    lv_obj_t *button_row = blusys::ui::row_create(col, {
        .gap     = blusys::ui::theme().spacing_sm,
        .padding = 0,
    });

    blusys::ui::button_create(button_row, {
        .label   = "Down",
        .variant = blusys::ui::button_variant::secondary,
        .on_press = +[](void *user_data) {
            auto *runtime = static_cast<blusys::framework::runtime *>(user_data);
            runtime->post_intent(blusys::framework::intent::decrement);
        },
        .user_data = g_runtime_ptr,
    });
    blusys::ui::button_create(button_row, {
        .label   = "Up",
        .variant = blusys::ui::button_variant::secondary,
        .on_press = +[](void *user_data) {
            auto *runtime = static_cast<blusys::framework::runtime *>(user_data);
            runtime->post_intent(blusys::framework::intent::increment);
        },
        .user_data = g_runtime_ptr,
    });
    blusys::ui::button_create(button_row, {
        .label   = "OK",
        .variant = blusys::ui::button_variant::primary,
        .on_press = +[](void *user_data) {
            auto *runtime = static_cast<blusys::framework::runtime *>(user_data);
            runtime->post_intent(blusys::framework::intent::confirm);
        },
        .user_data = g_runtime_ptr,
    });

    g_state.toast = blusys::ui::overlay_create(screen, {
        .text        = "Confirmed",
        .duration_ms = 1500,
        .on_hidden   = nullptr,
        .user_data   = nullptr,
    });

    // ---- spine wiring -------------------------------------------------------

    g_controller.bind(&g_state);
    g_route_sink.bind(&g_state);
    g_runtime.register_feedback_sink(&g_feedback_sink);

    const blusys_err_t init_err = g_runtime.init(&g_controller, &g_route_sink, 10);
    if (init_err != BLUSYS_OK) {
        BLUSYS_LOGE(kTag, "runtime.init failed: %d", static_cast<int>(init_err));
        return;
    }

    // ---- encoder focus traversal across the screen --------------------------

    lv_group_t *encoder_group = blusys::ui::create_encoder_group();
    blusys::ui::auto_focus_screen(screen, encoder_group);

    // ---- simulate a sequence of encoder events ------------------------------
    //
    // No real encoder is wired in this example. Posting intents directly is
    // exactly what an encoder indev callback would do, so this is a faithful
    // exercise of the spine. The slider value should advance by 3 steps
    // (kSliderInit + 3*kSliderStep) and the overlay should be shown via the
    // route command path.

    BLUSYS_LOGI(kTag, "initial slider = %ld",
                static_cast<long>(blusys::ui::slider_get_value(g_state.slider)));

    g_runtime.post_intent(blusys::framework::intent::increment);
    g_runtime.post_intent(blusys::framework::intent::increment);
    g_runtime.post_intent(blusys::framework::intent::increment);
    g_runtime.post_intent(blusys::framework::intent::confirm);

    g_runtime.step(0);

    BLUSYS_LOGI(kTag, "final slider = %ld",
                static_cast<long>(blusys::ui::slider_get_value(g_state.slider)));
    BLUSYS_LOGI(kTag, "queued events=%u routes=%u",
                static_cast<unsigned>(g_runtime.queued_event_count()),
                static_cast<unsigned>(g_runtime.queued_route_count()));
    BLUSYS_LOGI(kTag, "screen=%p slider=%p toast=%p group=%p",
                static_cast<void *>(screen),
                static_cast<void *>(g_state.slider),
                static_cast<void *>(g_state.toast),
                static_cast<void *>(encoder_group));
}
