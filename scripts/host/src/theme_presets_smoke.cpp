// Host-only compile check: presets diverge on density/spacing and visual
// feedback timing resolves against theme tokens.

#include "blusys/framework/ui/style/presets.hpp"
#include "blusys/framework/feedback/presets.hpp"
#include "blusys/framework/ui/style/theme.hpp"
#include "blusys/framework/ui/style/interaction_effects.hpp"

#include <cstdlib>

int main()
{
    const auto &expressive = blusys::app::presets::expressive_dark();
    const auto &operational = blusys::app::presets::operational_light();

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

    blusys::ui::set_theme(expressive);
    const std::uint32_t flash_ms =
        blusys::ui::visual_flash_ms(blusys::framework::feedback_preset_expressive(),
                                      blusys::framework::feedback_pattern::click,
                                      blusys::ui::theme());
    if (flash_ms == 0) {
        return 5;
    }

    return 0;
}
