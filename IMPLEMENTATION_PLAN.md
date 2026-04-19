# Blusys Platform Refactor — Final Plan

Full-authority redesign. No downstream users, no production deployments, no compatibility burden. Optimize unapologetically for the next ten years of maintenance, not the past one.

## North Star

Blusys is an embedded application platform. Its users are app authors. Everything else is internal plumbing.

Four architectural commitments drive every decision below. Ordered by priority — if two commitments conflict, earlier wins.

### 1. The capability is the only extension unit

Capabilities are how features enter the platform. Drivers are leaves on hardware. Services are internal mid-layer wrappers. *Only capabilities are the public extension surface.*

A new feature = one capability header + one `*_host.cpp` + one `*_device.cpp`. No app-side changes required to register it. If adding a feature requires touching something outside a capability, the platform is wrong.

### 2. Typed effects replace the service locator

The reducer receives a typed effect dispatcher (`fx`), not a mutable `services()` bag. Each capability owns one effect namespace:

```cpp
fx.nav.push(Route::Settings);
fx.storage.put("theme", "dark");
fx.mqtt.publish("sensor/temp", payload);
```

Adding a capability adds one namespace. Effects are typed. There is no way to reach an "effect bag" generically — which is exactly the point.

### 3. One event pipeline

Every event — framework intent, input device, capability event — is a `blusys::event` on one bus. The app spec has one integration hook:

```cpp
auto on_event(event e, state s) -> std::optional<Action>;
```

That is the entire event-to-action surface. The old intent/event/discriminant split collapses into this one function. Inputs (buttons, encoder, touch) are capabilities that emit events on the same bus — no special input path.

### 4. Platform split at the HAL line, nowhere else

`#ifdef ESP_PLATFORM` appears only below the HAL public interface and inside `*_device.cpp` / `*_host.cpp` backends. Host and device are swappable HAL backends. Drivers, services, runtime, capabilities, UI, entry, tests — all run unchanged on both.

Anything above HAL that checks for platform is a bug to fix, not a pattern to replicate.

---

## Target Shape

```
components/blusys/
├─ include/blusys/
│  ├─ blusys.hpp            ← THE public header. App authors #include only this.
│  ├─ hal/                  ← stable C ABI; host + device backends
│  │  ├─ internal/          ← shared HAL helpers (callback lifecycle, locks, timeouts)
│  │  └─ ...                ← peripheral headers
│  ├─ drivers/              ← C, on top of HAL
│  ├─ services/             ← C, INTERNAL ONLY. Not reachable from app code.
│  │  ├─ net/               ← bootstrap: wifi, wifi_prov, bt, espnow, mesh
│  │  ├─ session/           ← long-lived: mqtt, ws
│  │  └─ storage/           ← fs adapters
│  └─ framework/            ← C++, the app-author surface
│     ├─ runtime/           ← reducer, action dispatch, effect system
│     ├─ events/            ← one bus, one event type, one dispatch (was engine/)
│     ├─ capabilities/      ← extension system
│     ├─ ui/                ← one navigation controller (root); screens as children
│     ├─ flows/             ← first-class spec inputs (boot, provisioning, OTA, ...)
│     ├─ entry/             ← one parameterized entry template
│     ├─ observe/           ← log, trace, error domains, counters, budgets
│     └─ test/              ← fakes, mocks, headless harness
└─ src/                     ← mirror
```

Directory renames are cheap compared to the meaning changes inside them. Most of the work is *what lives where*, not moving files.

---

## Baselines

Measured at start of refactor. Phase targets are differences against these numbers.

| Area | File | LOC | Notes |
|---|---|---:|---|
| HAL drain | `hal/gpio.c` | 396 | ~35 LOC drain pattern |
| HAL drain | `hal/timer.c` | 368 | ~30 LOC drain pattern |
| HAL drain | `hal/uart.c` | 854 | drain + large state machine |
| Drivers drain | `drivers/button.c` | 312 | drain at driver layer |
| Drivers drain | `drivers/encoder.c` | 608 | drain at driver layer |
| Storage | `services/storage/fs.c` | 284 | `build_path` line 22–29 |
| Storage | `services/storage/fatfs.c` | 361 | `build_path` line 30–43 |
| Net | `services/connectivity/wifi.c` | 790 | |
| Net | `services/connectivity/wifi_prov.c` | 414 | |
| Net | `services/connectivity/espnow.c` | 307 | |
| Net | `services/connectivity/wifi_mesh.c` | 492 | |
| Net | `services/connectivity/bluetooth.c` | 527 | |
| Net | `services/connectivity/ble_gatt.c` | 732 | |
| Net | `services/connectivity/ble_hid_device.c` | 1001 | |
| Net | `services/connectivity/usb_hid.c` | 811 | |
| Session | `services/protocol/mqtt.c` | 413 | |
| Session | `services/protocol/ws_client.c` | 519 | |
| Display | `drivers/display.c` | 474 | `flush_cb` ~84 LOC (lines 106–189) |
| Display | `drivers/lcd.c` | 1178 | correct boundary, no change |
| Entry | `framework/app/entry.hpp` | 472 | 8 macro variants (see P7) |
| Runtime | `framework/app/internal/app_runtime.hpp` | ~420 | dispatch at lines 313–382 |
| Services | `framework/app/services.hpp` | 101 | 4 pointer fields + enum overloads |
| Ctx | `framework/app/ctx.hpp` | 135 | `services()`, `get<T>()` O(n) |
| Events | `framework/capabilities/event.hpp` | 127 | 95 `capability_event_tag` enumerators |
| Events | `framework/capabilities/event_table.hpp` | 193 | ~60 mapping entries |
| Flows | `src/framework/flows/*` | 1113 | 9 files |
| Screens | `src/framework/ui/screens/*` | 290 | 3 files |

`#ifdef ESP_PLATFORM` occurrences above HAL: **26** (target: 0).
Capability classes with shared shell boilerplate: **13** (est. 12–15 LOC each = 160–195 LOC).

---

## What Gets Deleted

Every refactor fails by adding without removing. This one deletes first.

- `app_ctx::services()` — the locator. Gone.
- `app_services` as a product-facing type — gone. Runtime-internal only, then deleted when B-phases complete.
- The old intent/event mapper split — gone. One `on_event`.
- `k_capability_event_discriminant_unset` — gone with the discriminant.
- `#ifdef ESP_PLATFORM` in any header above `hal/` — gone.
- `entry.hpp` macro variants (all old host/device/dashboard shims) — gone. One interactive macro + one headless variant.
- `shell` as an active owner of chrome logic — gone. Passive presentation only.
- `screen_router` / `overlay_manager` / `focus_scope` as public types reachable from `services()` — gone. Private children of the navigation controller.
- `framework/engine/integration_dispatch` as separate from event dispatch — gone, merged into one event bus.
- `framework/engine/intent.hpp` as a distinct type — gone, intent becomes an event kind.
- Duplicate shell-chrome sync in `entry.hpp` — gone with the macros.
- Duplicate `build_path()` in `fs.c` + `fatfs.c` — gone.
- Duplicate callback-drain loops in `gpio.c` / `timer.c` / `button.c` / `encoder.c` / `uart.c` — gone, one helper.
- Inline panel-kind branching in `display.c::flush_cb` — gone, strategy helpers.
- Any header that re-exports things `blusys.hpp` should re-export — gone.

If a phase adds but doesn't delete, the phase is incomplete.

---

## Phases

Twelve phases. Each is one PR, each leaves `main` buildable. Targets are caps — miss one, reassess scope before merging.

### P0. Foundation — before any refactor touches behavior

**Purpose:** every later phase rides on these. Built first so the refactor never flies blind.

**P0 is not one PR; it is three scoped sub-PRs (P0a/b/c) followed by P0d. Each sub-PR stands alone and leaves `main` buildable.**

#### P0a. Error domains + structured log

- One error-domain enum, grown *lazily*. Each subsystem adds its domain (`err_domain_storage_fs`, `err_domain_hal_gpio`, …) in the phase that actually migrates it. `blusys_err_t` carries `(domain, code)` — 32-bit packed — not an opaque int.
- Structured log under `framework/observe/`: `BLUSYS_LOG(level, domain, fmt, ...)`. One output format across host and device. Replace ad-hoc `BLUSYS_LOGE`/`LOGI`/`LOGW` call sites in one subsystem (storage) as the validation.
- Trace macros that compile out on release.
- Counters (monotonic u32) addressable by domain.

**Target:** `storage` subsystem uses only structured log. The domain enum covers *migrated* subsystems only: `generic`, `framework.observe`, the two storage adapters, and whichever drivers P0b's OOM fixes touch. Further domains are added *as subsystems migrate* in P1 / P2 / P5. Pre-declared placeholders for subsystems that haven't moved are speculation and get rejected in review.
**Deletes:** ad-hoc `ESP_LOGx` wrappers; raw integer error returns in storage.

#### P0b. Memory + threading contracts

- Document and enforce per-platform budgets in `framework/observe/budget.hpp`: device heap target (e.g., 300 KB), LVGL buffer size, action queue depth (current: 16), event queue depth.
- Every allocation path has an owner; document who frees what and when. OOM policy: fail closed, log, surface a domain error — no silent returns.
- Threading contract: reducer runs on one task; effect handlers run on that task unless explicitly marked async; capability `poll()` is cooperative; ISR→task handoff uses a documented primitive. Write the contract in `docs/internals/threading.md`.

**Target:** `display.c:313–327` silent-OOM sites fixed. Threading doc exists and is linked from `blusys.hpp`.
**Deletes:** every silent-failure allocation path in drivers/framework.

#### P0c. Test harness

- Headless reducer runner under `framework/test/`.
- HAL fake backend that compiles with host build — fakes for every peripheral used by tests.
- Capability mocks (one canonical mock per capability kind).
- Host build becomes the fast iteration loop; device build remains the integration gate.

**Target:** headless reducer + 3 capability mocks + fake HAL for gpio/timer/uart compile and run one smoke test.
**Deletes:** scattered ad-hoc host stubs in source files.

#### P0d. Lint guards + CI wiring

Under `scripts/`:

- `check-no-service-locator.sh` — greps for `ctx.services()`.
- `check-no-platform-ifdef-above-hal.sh` — bans `#ifdef ESP_PLATFORM` outside `hal/` and `*_device.cpp`/`*_host.cpp`.
- `check-public-api.sh` — bans non-`blusys.hpp` app-facing includes in examples.
- `check-capability-shape.sh` — enforces capability contract (kind_value + one header + one `*_host.cpp` + one `*_device.cpp`).
- `check-service-access.sh` — `services/*` reachable only from `framework/capabilities/` and `framework/runtime/`.

Each guard runs on every PR. Host build + one device build gate merge. Guards initially pass trivially; they activate as phases land.

**Target:** all five guards exist in CI; host + device builds gate merge.

### P1. HAL hardening

**Purpose:** make the HAL a real contract, not an accidental one.

- Formalize the C ABI in `include/blusys/hal/`. Every peripheral has: types + functions + error domain (from P0a). No macros leaking implementation.
- **Shared callback-lifecycle helper** under `hal/internal/callback_lifecycle.h` — the `callback_active` counter + `closing` flag + critical-section-guarded `try_enter` + `wait_for_idle` drain. Apply to `gpio.c`, `timer.c`, `uart.c`, `button.c`, `encoder.c`.
- **Host HAL backend** — fakes for every peripheral used by tests (extends P0c's seed fakes). Lives alongside the device backend. Selected by build system, not by `#ifdef` in source.

**Targets:** `gpio.c` + `timer.c` + `button.c` + `encoder.c` + `uart.c` lose ≥150 LOC combined against baseline (current sum: 2538 LOC; post: ≤2388). One drain implementation. Host fakes compile. `check-no-platform-ifdef-above-hal.sh` passes.

**Deletes:** all per-peripheral drain loops; all ad-hoc host stubs scattered through source.

### P2. Services internalized

**Purpose:** services become internal plumbing. Nothing in `framework/` or app code imports them directly.

- **Directory split:** `services/{net,session,storage}/`. Move files, update includes.
- **Storage helper** consolidating `build_path`, lock, CRUD, `blusys_translate_esp_err`. `fs.c` + `fatfs.c` become thin backend adapters.
- **Net bootstrap helper** (NVS + event loop + netif + singleton guard + shutdown ordering) for `wifi.c`, `wifi_prov.c`, `espnow.c`, `wifi_mesh.c`, `bluetooth.c`.
- **Session helper** (EventGroup + connected/closing + worker-task + timeout) for `mqtt.c`, `ws_client.c`. **Test prerequisite:** connect-success, connect-timeout, disconnect-while-inflight, reconnect tests exist and pass *before* the helper lands.
- **Access rule:** services are reachable only by capabilities (P5). Lint enforces it once P5 lands.

**Targets:** `fs.c` + `fatfs.c` ≤ 580 LOC combined (from 645). Net files lose ≥20% combined (from 5593 → ≤4475). Session files lose ≥25% combined (from 932 → ≤699).

**Deletes:** parallel CRUD wrappers; duplicate bootstrap scaffolding; duplicate session state machines.

**Why P2 before P3.** P3 wires effects to service backends. If services are still in their current scattered shape, P3 has to pick one of (a) reach into messy internals, (b) refactor services mid-phase. Pre-shaping services into clean helpers means P3 only wires; it never cleans. The alternative ordering (P3→P2) costs a double-migration of every effect handler.

### P3. Effect system — the architectural core

**Purpose:** replace `ctx.services()` with typed effects. This is the hinge of the whole refactor.

#### Design

`fx<App>` is compile-time assembled. Each capability declares its effect namespace via a nested `effects` type. The runtime instantiates one `fx` per app and injects it into the reducer.

#### POC

Capability declares its namespace:

```cpp
// framework/capabilities/storage.hpp
class storage_capability final : public capability_base {
public:
  static constexpr capability_kind kind_value = capability_kind::storage;
  static constexpr std::string_view fx_name = "storage";

  struct effects {
    storage_capability *self;
    blusys_err_t put(std::string_view key, std::span<const std::byte> value);
    blusys_err_t get(std::string_view key, std::span<std::byte> out, size_t *n);
    blusys_err_t erase(std::string_view key);
  };

  // unchanged: start/poll/stop/status
};
```

Runtime assembles `fx`:

```cpp
// framework/runtime/fx.hpp
template <typename... Caps>
struct fx {
  // One member per capability, named from Cap::fx_name.
  // Generated via a registration macro emitted by BLUSYS_CAPABILITIES(...).
  typename Caps::effects ... ;   // illustrative; implementation uses a tuple
                                 // plus a code-gen or constexpr name→index map.
};

// framework/runtime/fx_builder.hpp
template <typename App>
auto build_fx(capability_list &caps) -> fx<Cap1, Cap2, ...>;
```

Reducer signature:

```cpp
auto reduce(State s, Action a, fx<App>& e) -> State;
// Effects requested through e; reducer stays pure w.r.t. state.
```

Call sites migrate verbatim:

```cpp
// before
ctx.services().navigate_to(route::settings);
// after
fx.nav.to(route::settings);
```

#### Build-before-PR gate

Land a **prototype branch** first with two capabilities (`nav`, `storage`) wired end-to-end, compiled on host, one smoke test green. Only then open the P3 PR that migrates all call sites.

#### Work

- Implement `fx<App>` assembly (template + `BLUSYS_CAPABILITIES(...)` macro).
- Migrate every call site in examples (6 apps) and `src/framework/ui/screens/` (3 screens) and `src/framework/flows/` (9 flows) in the same PR. No shim.
- Delete `app_services` from public headers. What remains internally is an implementation detail of `fx` assembly.
- Replace `ctx.get<T>()` O(n) scan with index-based lookup derived from `fx` assembly.

**Targets:** zero `ctx.services()` call sites in product-facing code. `check-no-service-locator.sh` passes. Every capability exposes exactly one effect namespace. `ctx.get<T>()` is O(1).

**Deletes:** `app_services`, `app_ctx::services()`, every `ctx.services().xxx()` call, `ctx.get<T>()` linear scan.

### P4. One event pipeline

**Purpose:** collapse the three-hook dispatch to one.

- Rename `framework/engine/` → `framework/events/`. One event type `blusys::event` with `(source, kind, id, payload)`. Framework intents (button/encoder/touch) become event kinds on the same bus.
- Spec exposes one hook: `auto on_event(event, state) -> std::optional<Action>;` Runtime does all translation before the hook sees the event.
- Input subsystem becomes capabilities that emit events. No special-case input path.
- Delete the old intent/event mapper split and discriminant. Collapse `app_runtime.hpp:313–382` to a single dispatch function.
- The existing `capability_event_tag` enum (95 values) and `event_table.hpp` (~60 mappings, binary-searched) stay — those are good. What dies is the *three-path* routing wrapped around them.

**Targets:** one dispatch function in `app_runtime`. Spec has one event hook. `framework/events/` has one public event type. Dispatch LOC reduced by ≥40% in `app_runtime.hpp`.

**Deletes:** the three spec hooks, the discriminant, the intent type as a distinct concept, the old `integration_dispatch` layer.

### P5. Capability shell + access contract

**Purpose:** capabilities are a real extension unit with a real contract, not a copy-pasted header pattern.

- **Shell:** a plain virtual `capability_base` providing `kind_value` / `kind()` / `status()` / effect-handler registration / event emission. Each capability subclass declares only capability-specific members. A CRTP base or a `BLUSYS_CAPABILITY(...)` registration macro is acceptable *only* if the plain-virtual implementation demonstrably cannot hit the 60-LOC-per-capability header target. Reach for template machinery or code-gen last, not first — compile errors on virtual bases are legible; compile errors on CRTP bases are not.
- **Platform split:** one header per capability, one `*_host.cpp` and one `*_device.cpp`. No `#ifdef ESP_PLATFORM` anywhere in the header. The 26 `#ifdef ESP_PLATFORM` occurrences above HAL are collapsed here.
- **Event map:** `capability_event_map.cpp` is the single registry for raw IDs. Capability headers reference it; nothing re-encodes numeric contracts.
- **Access contract:** capabilities are the only callers of `services/*`. `check-service-access.sh` enforces the boundary.
- **Extension template:** a documented 50-line template for "adding a new capability" plus one canonical example capability (`example_sensor`). This is the contract.

#### Canonical 50-line template

```cpp
// framework/capabilities/example_sensor.hpp
#pragma once
#include <blusys/framework/capabilities/capability_base.hpp>

namespace blusys {

struct example_sensor_config { uint32_t sample_interval_ms = 1000; };
struct example_sensor_status { bool running = false; int32_t last_value = 0; };

class example_sensor_capability : public capability_base {
public:
  static constexpr capability_kind  kind_value = capability_kind::example_sensor;
  static constexpr std::string_view fx_name    = "sensor";

  explicit example_sensor_capability(const example_sensor_config& cfg);

  struct effects {
    example_sensor_capability *self;
    blusys_err_t read(int32_t *out);
  };

  capability_kind        kind()   const override { return kind_value; }
  example_sensor_status  status() const          { return status_; }

  blusys_err_t start(runtime &) override;
  void         poll(uint32_t now_ms) override;
  void         stop() override;

private:
  example_sensor_config cfg_;
  example_sensor_status status_{};
};

}  // namespace blusys
```

Two more files: `example_sensor_host.cpp`, `example_sensor_device.cpp`. That's the whole contract.

**Targets:** each capability `*.hpp` ≤ 60 LOC of class shell. Zero `#ifdef ESP_PLATFORM` in capability headers. `check-capability-shape.sh` passes. Capability-shell boilerplate reduced by ≥100 LOC across the 13 existing capabilities.

**Deletes:** duplicated class shells; inline `#ifdef` bodies; per-capability event-ID redeclarations.

### P6. UI as a reactive tree

**Purpose:** one owner for navigation, overlays, focus, chrome. The UI has a root.

- Introduce `framework/ui/controller/` — one `navigation_controller` that owns route stack, overlay stack, focus scope, and shell chrome. `screen_router`, `overlay_manager`, `focus_scope`, `shell` become private implementation details.
- **Transition hook signature:**
  ```cpp
  struct transition { route prev; route next; transition_kind kind; };
  void navigation_controller::on_transition(transition t);   // single call site
  ```
  Back-visibility, tab highlight, title sync happen here. Nowhere else.
- `fx.nav.*` effects dispatch through the controller. Product code never touches router/overlay/focus directly.
- `shell` becomes passive presentation with no chrome logic of its own.
- **Screens migration:** the three `src/framework/ui/screens/*` files (`about.cpp`, `diagnostics.cpp`, `status.cpp`) are currently framework-owned built-ins. They stay framework-owned but route through `navigation_controller` like everything else; they are not capabilities.

**Targets:** zero direct access to `screen_router` / `overlay_manager` / `focus_scope` from product code (examples + capabilities). One transition hook; chrome updates occur only through it.

**Deletes:** direct access to UI internals; duplicate chrome sync paths; `shell`'s active chrome logic.

### P7. Entry consolidation + flows as spec inputs

**Purpose:** one way to launch a blusys app. Flows stop being framework internals and become composable spec inputs.

#### Entry

- **One primary template:** `BLUSYS_APP(spec)`. Platform, variant, host/device selection is resolved from `spec` at compile time.
- **One survivor for headless apps:** `BLUSYS_APP_HEADLESS(spec)`. Only retained because headless apps genuinely don't link LVGL — it's a link-time decision, not a runtime one.
- The other six macros (`INTERACTIVE`, `DASHBOARD`, `MAIN_HOST`, `MAIN_HOST_PROFILE`, `MAIN_DEVICE`, `MAIN_HEADLESS_PROFILE`) collapse into `spec` fields: `spec.profile`, `spec.host_title`, `spec.initial_flow`.
- Platform-specific init (display bring-up, input wiring, LVGL init) moves out of macro bodies and into `spec`-driven initialization inside the framework.

#### Flows

The nine flow files (1113 LOC) currently live in `src/framework/flows/` as framework-owned, hard-wired behaviors:

| Flow | LOC | Post-P7 role |
|---|---:|---|
| `boot.cpp` | 94 | Framework built-in; runs before spec's `initial_flow` |
| `connectivity_flow.cpp` | 137 | **Spec-selectable**; activated when a connectivity capability is present |
| `diagnostics_flow.cpp` | 108 | Spec-selectable; off by default |
| `error.cpp` | 97 | Framework built-in; always on |
| `loading.cpp` | 91 | Framework built-in; used by other flows |
| `ota_flow.cpp` | 111 | Spec-selectable; activated when `ota` capability present |
| `provisioning_flow.cpp` | 110 | Spec-selectable; activated when `wifi_prov` capability present |
| `settings.cpp` | 230 | Spec-selectable; default-on for apps with UI |
| `status.cpp` | 135 | Spec-selectable; default-on for apps with UI |

- Flows become first-class `app_spec` inputs: `spec.flows = {flow::connectivity, flow::ota, flow::settings};`
- Each flow is an object with `start(fx&)`, `event_filter`, `stop()`. Flow selection is a spec field, not a macro choice.
- The framework composes flows into the event pipeline at init.

**Targets:** macro variant count = 2 (primary + headless). Zero platform init in macro bodies. Every example switches to the single template. Every flow is reachable only via a spec field, never via direct include.

**Deletes:** six of eight entry macros; inline display/input init in entry; direct flow instantiation from examples.

### P8. Display flush cleanup

**Purpose:** local cleanup; the display↔lcd boundary is already correct.

- Extract named strategy helpers in `display.c` (mono-page pack, RGB565 swap, direct blit). `flush_cb()` body becomes a small dispatcher.
- Do not rearrange the `display.c` ↔ `lcd.c` boundary — `lcd.c` has no LVGL knowledge, `display.c` has no panel-setup code. That part is fine.

**Targets:** `flush_cb()` body ≤ 20 LOC (from ~84 LOC). Each panel-kind path is one named helper. `display.c` total LOC reduced by ≥15%.

**Deletes:** inline panel-kind branches inside `flush_cb`.

### P9. Public API lockdown + docs

**Purpose:** seal the surface so accretion doesn't start over.

- `blusys.hpp` re-exports exactly the app-author API. Anything not re-exported is internal.
- `check-public-api.sh` bans examples and external app code from including non-`blusys.hpp` headers.
- Docs rewritten to reflect the new model:
  - `docs/internals/architecture.md` — the four commitments, the directory shape.
  - `docs/internals/threading.md` — the threading contract (from P0b).
  - `docs/app/app-runtime-model.md` — reducer + `fx` + events.
  - `docs/app/capability-composition.md` — the capability contract, 50-line template, canonical example.
  - `docs/app/adding-a-capability.md` — step-by-step for the only extension unit.
  - `docs/app/adding-a-flow.md` — flows as spec inputs.
  - `docs/app/memory-and-budgets.md` — per-platform budgets from P0b.
- Delete docs describing the old split, old bridges, old macros.

**Targets:** `blusys.hpp` is the only public include used by examples. Docs match code exactly.

**Deletes:** every doc page describing pre-refactor state.

### P10. Persistence & configuration

**Purpose:** make the state that survives reboots first-class. Currently implicit, which means it will break on real firmware updates.

- One `persistence_capability` owns NVS namespaces and key→domain routing. All WiFi creds, OTA status, provisioning state, app settings go through it.
- **Schemas in code, not yaml.** Each capability owns its persistence schema *in its own header* — a typed struct plus a `constexpr uint32_t schema_version`. `persistence_capability` discovers schemas at compile time through the capability list; there is no separate manifest to keep in sync.
- **Migration hook:** each capability declares `migrate_from(prev_version, raw_bytes)`. Framework invokes on boot, logs every migration via the structured log.
- **App settings API:** `fx.settings.get<T>(key)` / `fx.settings.put(key, value)` with typed schemas registered at compile time.

**Targets:** one persistence path; no service directly calling `nvs_*`. Every persisted key reached through `persistence_capability`. Migrations run on boot, logged via structured log. A capability's schema is declared exactly once — in its header — and the framework picks it up without extra plumbing.

**Deletes:** scattered `nvs_*` call sites; duplicate NVS namespace declarations.

**Not doing:** a central `inventory.yml` key registry with lint enforcement. Forcing every persisted key to be declared in two places (code + yaml) is a double-edit tax that only pays off past ~20 persisted keys or when cross-capability key collisions become a real incident. Capability-owned schema versions give us migration guarantees without that tax. Revisit if either condition arises.

### P11. Observability maturity

**Purpose:** P0a shipped the primitives. P11 makes them useful in production.

- `diagnostics_capability` exposes log ring, counter snapshot, last-error-per-domain through `fx.diag.*`.
- Crash path: on panic, dump last N log lines + counter snapshot to a dedicated NVS namespace. Read on next boot, surface through `diagnostics_capability`.
- Build-info capability: firmware version, build host, commit hash, feature flags — all addressable through `fx`.
- One diagnostics screen in `framework/ui/screens/` (already exists as `diagnostics.cpp`) becomes the uniform surface.

**Targets:** every domain has a counter. Last-error-per-domain survives reboot. Diagnostics screen reads exclusively through `fx.diag.*`.

**Deletes:** ad-hoc `printf` diagnostics; direct `ESP_LOGx` in product code.

### P12. Capability scaffolder + external-shape validation

**Purpose:** keep the "add a new capability" surface honest for current authors, and prove the P9 lockdown holds from outside the framework tree. This phase does **not** ship an SDK — it ships the technical guarantees an SDK would depend on.

- **Scaffolder:** `scripts/scaffold/new-capability.sh <name>` generates header + `*_host.cpp` + `*_device.cpp` from the 50-line template. Used internally; available externally.
- **External-shape example:** one example under `examples/external/` that includes only `<blusys.hpp>` and builds against the *installed* component (not via source-tree include paths). Compiled on host and device in CI — that's the lockdown proof.

**Targets:** external-shape example compiles on host and device using only `<blusys.hpp>`. Scaffolder produces a working capability in one command.

**Deletes:** example capabilities that cheat by reaching into framework internals.

**Not doing (yet):** a separate `docs/sdk/` tree, per-capability `sdk_stability: stable|experimental|internal` tags, or an external-author walkthrough. Those land *when there are actual external authors* — doing them now is guessing at their needs and creates a doc surface that rots without readers. The scaffolder plus external-shape example give us the technical guarantees; the SDK story is a separate product that picks them up later.

---

## Track Structure

```
P0a ─┐
P0b ─┼─ P0d ─┐
P0c ─┘        │
              ├─ P1 ── P2 ── P3 ── P4 ── P5 ── P6 ── P7 ── P8 ── P9 ── P10 ── P11 ── P12
              │
         (P0a/b/c land in parallel; P0d lint wiring gates them together)
```

- **P0a/b/c** are independently landable in any order. P0d (lint + CI) lands after all three.
- **P1–P2** are infrastructure. Low risk. Each independently landable.
- **P3–P5** are the architectural core. P3 (effects) unblocks everything above it. P3 **requires** a prototype branch (nav + storage wired) before the migration PR.
- **P6–P7** are product-facing. Depend on P3 for `fx.nav` and on P4 for the event hook.
- **P8–P9** are sealing and cleanup.
- **P10–P12** are maturity. They are not optional — without them, the platform can't ship — but they follow a working P9.

Recommended: serialize P1 onward. Parallelism costs more in coordination than it saves in wall time at this scope.

---

## Invariants enforced by CI after P9

These are the guarantees the refactor produces:

1. No `ctx.services()` anywhere in the repo.
2. No `#ifdef ESP_PLATFORM` outside `components/blusys/*/hal/` or `*_device.cpp`/`*_host.cpp` backend files.
3. Examples include only `<blusys.hpp>`.
4. Every capability has exactly one header + one `*_host.cpp` + one `*_device.cpp`.
5. Every service file is reached only from `framework/capabilities/` or `framework/runtime/`.
6. Host build + one device build pass on every PR.
7. Layering, mirror, namespace, host-bridge, public-API, service-locator, platform-ifdef, capability-shape, service-access checks all pass.
8. After P10: every persisted key is reached through `persistence_capability`; no service calls `nvs_*` directly.
9. After P12: `examples/external/*` compiles on host and device using only `<blusys.hpp>` against the *installed* component.

If any invariant is violated, the violation — not an exception — is the fix.

---

## What I am not doing and why

- **Not rewriting the reducer pattern.** It's right for this class of platform. Typed effects + one event bus are the cleanup; the core idea stays.
- **Not switching away from LVGL.** Rendering is a solved problem here; changing it would be churn without benefit.
- **Not introducing coroutines / async.** Fixed-capacity, synchronous dispatch, explicit event bus — the whole platform is easier to reason about this way. Don't add magic.
- **Not building a plugin loader.** Capabilities are compile-time. Dynamic loading is a different product; don't pay for flexibility you don't use.
- **Not versioning the public API.** No users, no version. When users arrive (P12), then version — and only then.
- **Not keeping the shim for `ctx.services()`.** One PR migrates every call site. A shim would extend the migration window indefinitely.
- **Not creating a yaml-backed persistence key registry.** Capability-owned schema versions in code give us migration guarantees without forcing every persisted key to be declared in two places. Revisit past ~20 persisted keys or a real cross-capability collision.
- **Not pre-declaring error domains for subsystems that haven't migrated.** The domain enum grows as subsystems move (P0a → P1 → P2 → P5). Declaring 50+ placeholders on day one is speculation; it ages into an enum of things no one uses.
- **Not reaching for CRTP or code-gen macros before a plain virtual base fails.** Template machinery is a real cost — slower builds, worse error messages, higher cognitive load on every reader. Default to the boring shape; escalate only if the 60-LOC-per-capability header target cannot otherwise be hit.
- **Not building an external-author SDK surface before external authors exist.** The scaffolder and external-shape example (P12) prove the lockdown and the ergonomics. A `docs/sdk/` tree, `sdk_stability` tags, and a curated walkthrough land when real demand shows up, not in anticipation of it.

---

## Exit criteria

- All four architectural commitments hold in code, enforced by CI.
- All "What Gets Deleted" items are actually deleted.
- All phase LOC and count targets are met against the baselines above.
- Adding a new capability takes one header + one `*_host.cpp` + one `*_device.cpp` and nothing else — verified by the scaffolder in P12.
- Host build runs the full reducer + capability test suite with fake HAL.
- `blusys.hpp` is the only public header an app author includes.
- Every persisted key reaches NVS through `persistence_capability`; capability-owned schema versions drive migrations on boot, logged via structured log.
- Diagnostics and last-error-per-domain survive reboot.
- `examples/external/*` compiles from outside the framework tree against the installed component using only `<blusys.hpp>`.
- Docs describe the current architecture, not the history.

This is the plan. Start at P0a.
