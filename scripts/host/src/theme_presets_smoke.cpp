// Host-only compile check: presets diverge on density/spacing and visual
// feedback timing resolves against theme tokens.

#include "blusys/framework/ui/style/presets.hpp"
#include "blusys/framework/feedback/presets.hpp"
#include "blusys/framework/ui/style/theme.hpp"
#include "blusys/framework/ui/style/interaction_effects.hpp"

#include <cstdlib>

int main()
{
    const auto &expressive = blusys::presets::expressive_dark();
    const auto &operational = blusys::presets::operational_light();
    const auto &compact = blusys::presets::compact_dark();

    if (expressive.design_w == 0 || operational.design_w == 0 ||
        expressive.design_w == operational.design_w) {
        return 1;
    }
    if (expressive.touch_target_min <= operational.touch_target_min) {
        return 2;
    }
    if (expressive.spacing_md == operational.spacing_md) {
        return 3;
    }
    if (expressive.text_letter_space_body == operational.text_letter_space_body) {
        return 4;
    }
    if (expressive.feedback_voice == operational.feedback_voice) {
        return 6;
    }
    if (compact.color_background != expressive.color_background) {
        return 7;
    }
    if (compact.color_background == operational.color_background) {
        return 8;
    }

    blusys::set_theme(expressive);
    const std::uint32_t flash_ms =
        blusys::visual_flash_ms(blusys::feedback_preset_expressive(),
                                      blusys::feedback_pattern::click,
                                      blusys::theme());
    if (flash_ms == 0) {
        return 5;
    }

    return 0;
}
