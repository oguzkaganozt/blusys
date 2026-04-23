# Authoring a capability

Framework-owned runtime module (connectivity, storage, OTA, …): **`start` / `poll` / `stop`**, **integration events**, and **`app_spec` config** — not direct product calls. **Using** built-ins only: [Capabilities](capabilities.md) and [Capability composition](capability-composition.md) (this page is for **adding** one to the tree).

## Contract

Every capability derives from `capability_base` in `blusys/framework/capabilities/capability.hpp`:

```cpp
class capability_base {
public:
    virtual ~capability_base() = default;
    virtual capability_kind kind() const = 0;
    virtual blusys_err_t start(blusys::runtime &rt) = 0;
    virtual void poll(std::uint32_t now_ms) = 0;
    virtual void stop() = 0;

protected:
    void post_integration_event(std::uint32_t event_id,
                                std::uint32_t event_code = 0,
                                const void *payload = nullptr);
    blusys::runtime *rt_ = nullptr;
};
```

`start()` is called once before `on_init`; `poll()` runs from the runtime step; `stop()` is called on shutdown. The base class provides `post_integration_event()` — always emit through it so the runtime ABI stays in one place.

## Reserved event-ID ranges

Each capability owns a 256-wide event-ID range (from `capability.hpp`):

| Range | Owner |
|-------|-------|
| `0x0100–0x01FF` | connectivity |
| `0x0200–0x02FF` | storage |
| `0x0300–0x03FF` | bluetooth |
| `0x0400–0x04FF` | ota |
| `0x0500–0x05FF` | diagnostics |
| `0x0600–0x06FF` | telemetry |
| `0x0700–0x07FF` | provisioning |
| `0x0800–0x08FF` | mqtt (SDL host) |
| `0x0900–0x09FF` | product custom |
| `0x0A00–0x0AFF` | lan_control |
| `0x0B00–0x0BFF` | usb |
| `0x0C00–0x0CFF` | ble_hid_device |
| `0x0D00–0x0DFF` | persistence |

Add a new kind to `capability_kind` and reserve a new range in the header comment when you add a capability to the framework. Product-custom capabilities live in the `0x0900` block and flow through `app_spec::on_event` as `integration_passthrough` until handled. For the one-off product-local shell, see [Product-custom capabilities](custom-capabilities.md).

## Shape to follow

Model new capabilities on `storage_capability` (`blusys/framework/capabilities/storage.hpp` + `src/framework/capabilities/storage.cpp`) — the smallest reference. Conventions:

1. **One header, two implementations.** Public header declares shared `<capability>_event` enum, a `<capability>_status` struct, and a `<capability>_config`. The class body gates device vs host with `#ifdef BLUSYS_DEVICE_BUILD`; device code lives in `src/framework/capabilities/<name>.cpp`, host stub in `<name>_host.cpp`.
2. **Declarative config only.** Take a `const <name>_config &` at construction. No runtime setters; changes happen via reconstruction on the `app_spec`.
3. **Strong event enum.** Define `enum class <name>_event : std::uint32_t { … , capability_ready = 0x0<N>FF }` with values drawn from the reserved range. Always emit the terminal `capability_ready` when the configured work is done.
4. **Queryable status.** Expose `const <name>_status &status() const`. Update the struct before posting the corresponding event so observers see consistent state.
5. **Direct handle access where useful.** Filesystem, socket, or protocol handles may be returned from accessors (see `storage_capability::spiffs()` / `fatfs()`); return `nullptr` when not configured.
6. **Emit via the base helper.** A thin `post_event(<name>_event)` forwarder that casts to `std::uint32_t` and calls `post_integration_event()` keeps call sites tidy.
7. **`start()` sets `rt_`, `stop()` clears it.** `post_integration_event` is null-safe; events fire only while the runtime is bound.
8. **`poll()` must be non-blocking.** Capabilities share the runtime thread; long work belongs in a worker task owned inside the capability.

## Wiring in the app entrypoint

Add the capability to the `app_spec`'s `capability_list`; map its events to product actions in `main/app_main.cpp` (or via `app_spec::on_event`). The reducer consumes those actions and never calls the capability directly.

Typed event helpers live in `blusys/framework/app/variant_dispatch.hpp` (`dispatch_variant`) — prefer those over raw integer compares when the event set grows.

## Validation

- Host stub must compile and emit `capability_ready` on first `poll()` so host-driven examples reach steady state without real hardware.
- Add a validation example under `examples/validation/` that composes only your capability; verify the event sequence and `status()` transitions.
- Update `docs/app/capabilities.md` with usage and `docs/internals/target-matrix.md` with any sdkconfig or managed-component requirements.
- Scaffold the header + host/device stubs with `scripts/scaffold/new-capability.sh <name>` when you want the canonical capability shell quickly.

## See also

- [Adding a capability](adding-a-capability.md) — repo steps for a new kind (header, host, device, register)
- [App Runtime Model](app-runtime-model.md) — queue, threading, drops
- [Product-custom capabilities](custom-capabilities.md) — product-local shell
- `components/blusys/include/blusys/framework/capabilities/capability.hpp` — `capability_base`
- `components/blusys/include/blusys/framework/capabilities/storage.hpp` — smallest reference; `scripts/scaffold/new-capability.sh` for a generated shell
