#pragma once

// Shell chrome from a logical device_profile — avoids duplicating shell_config
// assembly in integration/app_main.cpp for dashboard-class examples.

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include "blusys/framework/platform/layout_surface.hpp"
#include "blusys/framework/platform/profile.hpp"
#include "blusys/framework/ui/composition/shell.hpp"

namespace blusys {

[[nodiscard]] inline shell_config auto_shell(const device_profile &prof, const char *title)
{
    const auto            chrome = layout::shell_chrome_for(prof);
    shell_config c{};
    c.header.enabled   = true;
    c.header.title     = title;
    c.status.enabled   = chrome.status_enabled;
    c.tabs.enabled     = chrome.tabs_enabled;
    return c;
}

/// Like auto_shell, but tightens header/status/tab heights on compact surfaces (mqtt dashboard).
[[nodiscard]] inline shell_config auto_shell_adaptive(const device_profile &prof,
                                                            const char           *title)
{
    shell_config c = auto_shell(prof, title);
    const bool           compact_chrome =
        (prof.lcd.height <= 280) || (prof.lcd.width <= 360 && prof.lcd.height <= 300);
    if (compact_chrome) {
        c.header.height = 30;
        c.status.height = 16;
        c.tabs.height   = 28;
    }
    return c;
}

}  // namespace blusys

#endif  // BLUSYS_FRAMEWORK_HAS_UI
