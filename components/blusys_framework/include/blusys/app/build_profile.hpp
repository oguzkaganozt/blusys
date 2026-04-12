#pragma once

// Archetype-neutral aliases for the stock interactive ST7735 / ST7789 / SDL host matrix.
// Prefer these names in new integration entry files; `reference_build_profile.hpp` remains
// the implementation source.

#include "blusys/app/auto_profile.hpp"
#include "blusys/app/reference_build_profile.hpp"

namespace blusys::app::build_profile {

[[nodiscard]] inline const char *version_string()
{
    return reference_build::build_version_string();
}

#if defined(ESP_PLATFORM)

[[nodiscard]] inline device_profile device_profile_for_interactive()
{
    return auto_profile_interactive();
}

#else

[[nodiscard]] inline device_profile host_logical_profile()
{
    return auto_profile_interactive();
}

#endif

[[nodiscard]] inline const char *panel_label()
{
    return reference_build::interactive_display_label();
}

[[nodiscard]] inline const char *hardware_label()
{
    return reference_build::interactive_hardware_label();
}

#if defined(BLUSYS_FRAMEWORK_HAS_UI)

[[nodiscard]] inline view::shell_config shell_config_for_device_profile(const device_profile &prof)
{
    return reference_build::interactive_shell_config_for_profile(prof);
}

#if !defined(ESP_PLATFORM)
[[nodiscard]] inline host_profile host_window(const char *window_title)
{
    return auto_host_profile_interactive(window_title);
}
#endif

#endif

}  // namespace blusys::app::build_profile
