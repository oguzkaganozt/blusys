#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/screens/about_screen.hpp"
#include "blusys/app/capabilities/diagnostics.hpp"
#include "blusys/app/view/page.hpp"
#include "blusys/framework/ui/theme.hpp"
#include "blusys/framework/ui/primitives/key_value.hpp"
#include "blusys/framework/ui/primitives/divider.hpp"
#include "blusys/framework/ui/widgets/card/card.hpp"

#include <cstdio>

namespace blusys::app::screens {

lv_obj_t *about_screen_create(app_ctx &ctx, const void *params,
                               lv_group_t **group_out)
{
    const auto *cfg = static_cast<const about_screen_config *>(params);
    const auto &t   = blusys::ui::theme();

    auto page = view::page_create({.scrollable = true});

    // Title.
    lv_obj_t *title = lv_label_create(page.content);
    lv_label_set_text(title, "About");
    lv_obj_set_style_text_font(title, t.font_title, 0);
    lv_obj_set_style_text_color(title, t.color_on_surface, 0);
    lv_obj_set_width(title, LV_PCT(100));

    // Device info card.
    blusys::ui::card_config card_cfg{};
    lv_obj_t *card = blusys::ui::card_create(page.content, card_cfg);
    lv_obj_set_width(card, LV_PCT(100));

    if (cfg != nullptr) {
        if (cfg->product_name != nullptr) {
            blusys::ui::key_value_create(card, {
                .key = "Product", .value = cfg->product_name,
            });
        }
        if (cfg->firmware_version != nullptr) {
            blusys::ui::key_value_create(card, {
                .key = "Firmware", .value = cfg->firmware_version,
            });
        }
        if (cfg->hardware_version != nullptr) {
            blusys::ui::key_value_create(card, {
                .key = "Hardware", .value = cfg->hardware_version,
            });
        }
        if (cfg->serial_number != nullptr) {
            blusys::ui::key_value_create(card, {
                .key = "Serial", .value = cfg->serial_number,
            });
        }
        if (cfg->build_date != nullptr) {
            blusys::ui::key_value_create(card, {
                .key = "Built", .value = cfg->build_date,
            });
        }

        // Extra fields.
        for (std::size_t i = 0; i < cfg->extra_count; ++i) {
            const auto &extra = cfg->extras[i];
            if (extra.key != nullptr) {
                blusys::ui::key_value_create(card, {
                    .key = extra.key, .value = extra.value,
                });
            }
        }

        if (cfg->fill_diagnostics_from_ctx) {
            const auto *diag = ctx.diagnostics();
            if (diag != nullptr && diag->capability_ready) {
                const auto &sn = diag->last_snapshot;
                char buf[48];

                blusys::ui::key_value_create(card, {
                    .key = "Chip", .value = sn.chip_model,
                });

                std::snprintf(buf, sizeof(buf), "%lu",
                              static_cast<unsigned long>(sn.free_heap));
                blusys::ui::key_value_create(card, {
                    .key = "Free heap (bytes)", .value = buf,
                });

                std::snprintf(buf, sizeof(buf), "%lu",
                              static_cast<unsigned long>(sn.min_free_heap));
                blusys::ui::key_value_create(card, {
                    .key = "Min free heap", .value = buf,
                });

                std::snprintf(buf, sizeof(buf), "%lu ms",
                              static_cast<unsigned long>(sn.uptime_ms));
                blusys::ui::key_value_create(card, {
                    .key = "Uptime", .value = buf,
                });

                blusys::ui::key_value_create(card, {
                    .key = "IDF", .value = sn.idf_version,
                });
            }
        }
    }

    if (group_out != nullptr) {
        *group_out = page.group;
    }
    return page.screen;
}

}  // namespace blusys::app::screens

#endif  // BLUSYS_FRAMEWORK_HAS_UI
