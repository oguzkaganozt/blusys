#include "blusys/framework/ui/icons/icon_set_minimal.hpp"

#include "lvgl.h"

namespace blusys::ui {

const icon_set &icon_set_minimal()
{
    static const icon_set set{
        .font      = LV_FONT_DEFAULT,

        .wifi      = LV_SYMBOL_WIFI,
        .bluetooth = LV_SYMBOL_BLUETOOTH,
        .ok        = LV_SYMBOL_OK,
        .warning   = LV_SYMBOL_WARNING,
        .error     = LV_SYMBOL_CLOSE,
        .arrow_up  = LV_SYMBOL_UP,
        .arrow_down  = LV_SYMBOL_DOWN,
        .arrow_back  = LV_SYMBOL_LEFT,
        .settings  = LV_SYMBOL_SETTINGS,
        .info      = LV_SYMBOL_LIST,
        .ota       = LV_SYMBOL_DOWNLOAD,
        .battery   = LV_SYMBOL_BATTERY_FULL,
        .signal    = LV_SYMBOL_BARS,
        .power     = LV_SYMBOL_POWER,
        .refresh   = LV_SYMBOL_REFRESH,
        .close     = LV_SYMBOL_CLOSE,

        .menu      = LV_SYMBOL_LIST,
        .more      = LV_SYMBOL_PLUS,
        .nav_next  = LV_SYMBOL_RIGHT,
        .nav_prev  = LV_SYMBOL_LEFT,
    };
    return set;
}

}  // namespace blusys::ui
