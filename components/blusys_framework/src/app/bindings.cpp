#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/app/view/bindings.hpp"
#include "blusys/framework/ui/widgets/slider/slider.hpp"
#include "blusys/framework/ui/widgets/toggle/toggle.hpp"

namespace blusys::app::view {

void set_text(lv_obj_t *label, const char *text)
{
    if (label != nullptr && text != nullptr) {
        lv_label_set_text(label, text);
    }
}

void set_value(lv_obj_t *slider, std::int32_t value)
{
    if (slider != nullptr) {
        blusys::ui::slider_set_value(slider, value);
    }
}

std::int32_t get_value(lv_obj_t *slider)
{
    if (slider != nullptr) {
        return blusys::ui::slider_get_value(slider);
    }
    return 0;
}

void set_range(lv_obj_t *slider, std::int32_t min, std::int32_t max)
{
    if (slider != nullptr) {
        blusys::ui::slider_set_range(slider, min, max);
    }
}

void set_checked(lv_obj_t *toggle, bool on)
{
    if (toggle != nullptr) {
        blusys::ui::toggle_set_state(toggle, on);
    }
}

bool get_checked(lv_obj_t *toggle)
{
    if (toggle != nullptr) {
        return blusys::ui::toggle_get_state(toggle);
    }
    return false;
}

void set_enabled(lv_obj_t *widget, bool enabled)
{
    if (widget == nullptr) {
        return;
    }
    if (enabled) {
        lv_obj_clear_state(widget, LV_STATE_DISABLED);
    } else {
        lv_obj_add_state(widget, LV_STATE_DISABLED);
    }
}

void set_visible(lv_obj_t *widget, bool visible)
{
    if (widget == nullptr) {
        return;
    }
    if (visible) {
        lv_obj_clear_flag(widget, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_add_flag(widget, LV_OBJ_FLAG_HIDDEN);
    }
}

}  // namespace blusys::app::view

#endif  // BLUSYS_FRAMEWORK_HAS_UI
