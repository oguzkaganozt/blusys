#pragma once
#include "blusys/blusys.hpp"


#include "core/atlas_logic.hpp"

#include <cstddef>
#include <cstdint>

namespace atlas_example {

class atlas_capability;

// Extensible command dispatch. Default handlers ship for
// ping / reboot / factory_reset / get_diagnostics; products register
// additional types via `on_command(type, fn, ctx)` or override the unknown-
// type fallback via `set_default_command_handler(...)`.
//
// Lifetime rules for `atlas_command`:
//   - `type` and `command_id` are owned (copied from the payload).
//   - `payload_json` is BORROWED — it points into the inbound
//     `mqtt_message` buffer and is valid ONLY for the duration of the
//     handler call. Use `copy_payload()` if you need to defer work past
//     the current MQTT drain pass.
struct atlas_command {
    char               type[32]       = {};
    char               command_id[64] = {};
    const char        *payload_json   = nullptr;
    std::size_t        payload_len    = 0;
    atlas_capability  *cap            = nullptr;

    // Handler helpers. `ack()` is idempotent — the framework already
    // published an ack before dispatch, so calls here produce harmless
    // duplicates that the backend deduplicates.
    blusys_err_t ack() const;
    blusys_err_t fail(const char *error_message) const;
    blusys_err_t result(const char *json, std::size_t len) const;

    // Copy the borrowed payload into caller-owned storage. Returns bytes
    // written (excluding nul). The destination is always nul-terminated
    // when `dst_sz > 0`.
    std::size_t copy_payload(char *dst, std::size_t dst_sz) const;
};

using atlas_command_fn = void (*)(const atlas_command &cmd, void *user_ctx);

// Identity and firmware metadata. Auth + broker transport live in the
// shared `mqtt_capability` the owner composes alongside this one.
struct atlas_config {
    const char                          *device_id        = nullptr;
    const char                          *firmware_version = "1.0.0";

    // Optional — when non-null, the default `get_diagnostics` handler
    // includes `rssi` in its result payload. No other code paths depend
    // on this pointer.
    blusys::connectivity_capability     *connectivity     = nullptr;
};

// Atlas-protocol adapter. Stays in product code because it encodes the
// Atlas wire conventions — topic layout, retained state shape, ota
// announcement schema — not a reusable framework primitive.
//
// Composition contract:
//   - `mqtt` must be pre-populated with per-device auth, CA bundle, and
//     broker URL (atlas_main.cpp wires that). `atlas_capability` adds its
//     subscriptions at runtime once MQTT is connected, and installs itself
//     as `mqtt`'s direct message handler.
//   - `ota` is invoked via `request_update(url)` when an Atlas OTA
//     announcement arrives — the URL is pulled from the announcement JSON
//     rather than baked into `ota_config`.
class atlas_capability final : public blusys::capability_base {
public:
    atlas_capability(const atlas_config &cfg,
                     blusys::mqtt_capability *mqtt,
                     blusys::ota_capability  *ota);

    atlas_capability(const atlas_capability &)            = delete;
    atlas_capability &operator=(const atlas_capability &) = delete;

    [[nodiscard]] blusys::capability_kind kind() const override
    {
        return blusys::capability_kind::custom;
    }

    blusys_err_t start(blusys::runtime &rt) override;
    void         poll(std::uint32_t now_ms) override;
    void         stop() override;

    // App-thread publish helpers (reducer / on_tick).
    blusys_err_t publish_state(const char *json, std::size_t len);
    blusys_err_t publish_heartbeat(const char *json, std::size_t len);
    blusys_err_t publish_event(const char *json, std::size_t len);

    // Command result publish. `command_id` must be the value extracted
    // from the incoming down/command payload. `error_message` is ignored
    // on ack and included verbatim on fail (may be nullptr).
    blusys_err_t publish_command_ack(const char *command_id);
    blusys_err_t publish_command_fail(const char *command_id, const char *error_message);
    blusys_err_t publish_command_result(const char *command_id,
                                        const char *json, std::size_t len);

    // Register a handler for a command `type`. Last-write-wins by key —
    // re-registering overwrites (with a warning log). Returns
    // BLUSYS_ERR_NO_MEM when the 16-slot registry is full and the type
    // doesn't match an existing entry.
    blusys_err_t on_command(const char *type, atlas_command_fn fn, void *user_ctx);

    // Set the fallback invoked when an incoming command's `type` doesn't
    // match any registered handler. Default fallback publishes
    // `command_fail` with errorMessage="unknown_type".
    void set_default_command_handler(atlas_command_fn fn, void *user_ctx);

private:
    struct command_handler_entry {
        char             type[32]  = {};
        atlas_command_fn fn        = nullptr;
        void            *user_ctx  = nullptr;
        bool             in_use    = false;
    };

    enum class reboot_mode : std::uint8_t {
        none,
        reboot,
        factory_reset,
    };

    static void on_mqtt_message(const blusys::mqtt_message &msg, void *user_ctx);
    void        handle_mqtt_message(const blusys::mqtt_message &msg);
    void        on_mqtt_connected();

    const command_handler_entry *find_command_handler(const char *type) const;
    std::size_t list_supported_commands(char *dst, std::size_t dst_sz) const;

    // Built-in default handlers.
    static void builtin_ping(const atlas_command &cmd, void *user_ctx);
    static void builtin_reboot(const atlas_command &cmd, void *user_ctx);
    static void builtin_factory_reset(const atlas_command &cmd, void *user_ctx);
    static void builtin_get_diagnostics(const atlas_command &cmd, void *user_ctx);
    static void default_unknown_type(const atlas_command &cmd, void *user_ctx);

    void schedule_reboot(reboot_mode mode, std::uint32_t delay_ms);

    atlas_config              cfg_;
    blusys::mqtt_capability  *mqtt_ = nullptr;
    blusys::ota_capability   *ota_  = nullptr;

    bool was_connected_       = false;

    // Registry state. Array-of-entries is cheap enough for a 16-slot
    // linear scan at 1 cmd/sec.
    static constexpr std::size_t kMaxCommandHandlers = 16;
    command_handler_entry command_handlers_[kMaxCommandHandlers] = {};

    atlas_command_fn default_handler_fn_  = &atlas_capability::default_unknown_type;
    void            *default_handler_ctx_ = nullptr;

    // Deferred reboot/factory-reset so the `command_ack` has a chance to
    // leave the broker before the device restarts.
    reboot_mode   pending_reboot_mode_ = reboot_mode::none;
    std::uint32_t pending_reboot_at_ms_ = 0;
    std::uint32_t last_poll_now_ms_    = 0;

    char topic_up_state_[128]     = {};
    char topic_up_heartbeat_[128] = {};
    char topic_up_event_[128]     = {};
    char topic_down_command_[128] = {};
    char topic_down_state_[128]   = {};
    char topic_down_ota_[128]     = {};
};

}  // namespace atlas_example
