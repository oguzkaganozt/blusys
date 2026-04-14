#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/flows/boot.hpp"
#include "blusys/framework/ui/composition/page.hpp"
#include "blusys/framework/ui/style/theme.hpp"

namespace blusys::flows {

namespace {

struct splash_ctx {
    app_ctx *ctx;
    void (*on_complete)(app_ctx &ctx);
};

void splash_timer_cb(lv_timer_t *timer)
{
    auto *sc = static_cast<splash_ctx *>(lv_timer_get_user_data(timer));
    if (sc != nullptr && sc->on_complete != nullptr && sc->ctx != nullptr) {
        sc->on_complete(*sc->ctx);
    }
    lv_timer_delete(timer);
}

// Static storage for splash context (only one splash active at a time).
splash_ctx g_splash_ctx{};

}  // namespace

lv_obj_t *boot_splash_create(app_ctx &ctx, const void *params,
                              lv_group_t **group_out)
{
    const auto *cfg = static_cast<const boot_splash_config *>(params);
    const auto &t   = blusys::theme();

    auto page = page_create({.scrollable = false});

    lv_obj_set_style_bg_color(page.screen, t.color_background, 0);

    // Center content vertically.
    lv_obj_set_flex_align(page.content,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER,
                          LV_FLEX_ALIGN_CENTER);
    lv_obj_set_size(page.content, LV_PCT(100), LV_PCT(100));

    // Product name.
    if (cfg != nullptr && cfg->product_name != nullptr) {
        lv_obj_t *name = lv_label_create(page.content);
        lv_label_set_text(name, cfg->product_name);
        lv_obj_set_style_text_font(name, t.font_display, 0);
        lv_obj_set_style_text_color(name, t.color_on_surface, 0);
        lv_obj_set_style_text_align(name, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_width(name, LV_PCT(100));
    }

    // Tagline.
    if (cfg != nullptr && cfg->tagline != nullptr) {
        lv_obj_t *tag = lv_label_create(page.content);
        lv_label_set_text(tag, cfg->tagline);
        lv_obj_set_style_text_font(tag, t.font_body, 0);
        lv_obj_set_style_text_color(tag, t.color_on_surface, 0);
        lv_obj_set_style_text_opa(tag, LV_OPA_70, 0);
        lv_obj_set_style_text_align(tag, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_width(tag, LV_PCT(100));
    }

    // Version.
    if (cfg != nullptr && cfg->version != nullptr) {
        lv_obj_t *ver = lv_label_create(page.content);
        lv_label_set_text(ver, cfg->version);
        lv_obj_set_style_text_font(ver, t.font_body_sm, 0);
        lv_obj_set_style_text_color(ver, t.color_on_surface, 0);
        lv_obj_set_style_text_opa(ver, LV_OPA_50, 0);
        lv_obj_set_style_text_align(ver, LV_TEXT_ALIGN_CENTER, 0);
        lv_obj_set_width(ver, LV_PCT(100));
    }

    // Auto-dismiss timer.
    if (cfg != nullptr && cfg->on_complete != nullptr && cfg->display_ms > 0) {
        g_splash_ctx.ctx = &ctx;
        g_splash_ctx.on_complete = cfg->on_complete;
        lv_timer_create(splash_timer_cb, cfg->display_ms, &g_splash_ctx);
    }

    if (group_out != nullptr) {
        *group_out = page.group;
    }
    return page.screen;
}

}  // namespace blusys::flows

#endif  // BLUSYS_FRAMEWORK_HAS_UI
