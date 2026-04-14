#pragma once

#include "lvgl.h"

namespace blusys::fonts {

// Resolve Montserrat faces when enabled in lv_conf / sdkconfig; otherwise
// fall back to LV_FONT_DEFAULT so embedded targets stay small.

[[nodiscard]] inline const lv_font_t *montserrat_14()
{
#if defined(LV_FONT_MONTSERRAT_14) && LV_FONT_MONTSERRAT_14
    return &lv_font_montserrat_14;
#else
    return LV_FONT_DEFAULT;
#endif
}

[[nodiscard]] inline const lv_font_t *montserrat_20()
{
#if defined(LV_FONT_MONTSERRAT_20) && LV_FONT_MONTSERRAT_20
    return &lv_font_montserrat_20;
#else
    return montserrat_14();
#endif
}

}  // namespace blusys::fonts
