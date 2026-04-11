#pragma once

#ifdef BLUSYS_FRAMEWORK_HAS_UI

#include <cstddef>
#include <cstdint>

#include "lvgl.h"

namespace blusys::app { class app_ctx; }

namespace blusys::app::view {

class screen_registry;

// Shell header configuration.
struct shell_header_config {
    bool        enabled   = false;
    int         height    = -1;      // -1 = theme-derived
    const char *title     = nullptr; // initial title text
};

// Shell status bar configuration.
struct shell_status_config {
    bool enabled = false;
    int  height  = -1;  // -1 = theme-derived
};

// Shell tab bar configuration (tab items are set via shell_set_tabs).
struct shell_tab_config {
    bool enabled = false;
    int  height  = -1;  // -1 = theme-derived
};

// Top-level shell configuration. Pass a pointer to this in app_spec
// to enable the interaction shell.
struct shell_config {
    shell_header_config header;
    shell_status_config status;
    shell_tab_config    tabs;
};

// Tab item descriptor for shell tab bar.
static constexpr std::size_t kMaxTabs = 6;

struct shell_tab_item {
    const char   *label    = nullptr;
    std::uint32_t route_id = 0;
};

// The shell owns a persistent screen with optional header, status bar,
// content area, and tab bar. Screen content is swapped inside the
// content area on navigation — the shell surfaces persist.
//
// Tab bar (shell chrome) vs in-page tabs: shell tabs are top-level section
// switches (same stack, `navigate_to` per tab). The stock `tabs` widget
// is for content inside a single screen — do not mix the two roles.
struct shell {
    lv_obj_t *root         = nullptr; // the real lv_screen
    lv_obj_t *header       = nullptr; // persistent header (nullptr if disabled)
    lv_obj_t *header_title = nullptr; // title label inside header
    lv_obj_t *header_back  = nullptr; // back button inside header
    lv_obj_t *status       = nullptr; // persistent status surface (nullptr if disabled)
    lv_obj_t *content_area = nullptr; // container where screen content is placed
    lv_obj_t *tab_bar      = nullptr; // persistent tab bar (nullptr if disabled)

    shell_tab_item tabs[kMaxTabs] = {};
    std::size_t    tab_count      = 0;
    std::size_t    active_tab     = 0;
    app_ctx       *ctx            = nullptr; // bound context for tab navigation
};

// Create the shell from config. Does not load it.
shell shell_create(const shell_config &config);

// Load the shell as the active LVGL screen.
void shell_load(shell &s);

// Get the content area parent for placing screen content.
lv_obj_t *shell_content(shell &s);

// Update the header title text.
void shell_set_title(shell &s, const char *title);

// Show or hide the back button in the header.
void shell_set_back_visible(shell &s, bool visible);

// Get the status surface for adding status widgets.
lv_obj_t *shell_status_surface(shell &s);

// Register tab items. Must be called before shell_load().
// The tab bar's on_tab_changed fires ctx->navigate_to(route_id).
void shell_set_tabs(shell &s, const shell_tab_item *items, std::size_t count,
                    app_ctx *ctx);

// Set the active tab by index (updates visual highlight only).
void shell_set_active_tab(shell &s, std::size_t index);

// Match tab highlight (and optional header title) to the current nav stack.
// Walks from the top of the stack downward until a route matches a tab item.
void shell_sync_tabs_for_nav_stack(shell &s, const screen_registry &nav);

}  // namespace blusys::app::view

#endif  // BLUSYS_FRAMEWORK_HAS_UI
