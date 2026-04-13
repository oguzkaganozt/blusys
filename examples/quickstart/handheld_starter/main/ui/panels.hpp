#pragma once

// Product UI components: LVGL handles + sync from app_state. Defined only under ui/.

#include "blusys/app/view/bindings.hpp"

#include <cstdint>

#include "lvgl.h"

namespace handheld_starter {

struct app_state;

namespace ui {

struct ShellChrome {
    lv_obj_t *badge  = nullptr;
    lv_obj_t *detail = nullptr;

    void clear()
    {
        badge  = nullptr;
        detail = nullptr;
    }

    void sync(const app_state &state, const char *accent_line);
};

struct HomePanel {
    blusys::app::view::percent_output_cluster output{};
    lv_obj_t *preset_kv  = nullptr;
    lv_obj_t *hold_badge = nullptr;

    void clear()
    {
        output.gauge     = nullptr;
        output.level_bar = nullptr;
        output.vu_strip  = nullptr;
        preset_kv        = nullptr;
        hold_badge       = nullptr;
    }

    void sync(const app_state &state);
};

struct StatusPanel {
    lv_obj_t *setup_badge = nullptr;
    lv_obj_t *storage_kv  = nullptr;
    lv_obj_t *output_kv   = nullptr;
    lv_obj_t *hold_kv     = nullptr;

    void clear()
    {
        setup_badge = nullptr;
        storage_kv  = nullptr;
        output_kv   = nullptr;
        hold_kv     = nullptr;
    }

    void sync(const app_state &state);
};

// Called at end of update() — shell, route panels, provisioning setup screen.
void sync_all_panels(app_state &state);

}  // namespace ui
}  // namespace handheld_starter
