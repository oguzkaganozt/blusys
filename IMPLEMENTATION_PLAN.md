# Blusys Platform Refactor — Final Plan

Full-authority redesign. No downstream users, no production deployments, no compatibility burden. Optimize unapologetically for the next ten years of maintenance, not the past one.

## North Star

Blusys is an embedded application platform. Its users are app authors. Everything else is internal plumbing.

Four architectural commitments drive every decision below. Ordered by priority — if two commitments conflict, earlier wins.

### 1. The capability is the only extension unit

Capabilities are how features enter the platform. Drivers are leaves on hardware. Services are internal mid-layer wrappers. *Only capabilities are the public extension surface.*

A new feature = one capability file (+ one per-platform impl file). No app-side changes required to register it. If adding a feature requires touching something outside a capability, the platform is wrong.

### 2. Typed effects replace the service locator

The reducer receives a typed effect dispatcher (`fx`), not a mutable `services()` bag. Each capability owns one effect namespace. Calls look like:

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

That is the entire event-to-action surface. `map_intent` / `map_event` / `capability_event_discriminant` collapse into this one function. Inputs (buttons, encoder, touch) are capabilities that emit events on the same bus — no special input path.

### 4. Platform split at the HAL line, nowhere else

`#ifdef ESP_PLATFORM` appears in exactly one place: below the HAL public interface. Host and device are swappable HAL backends. Drivers, services, runtime, capabilities, UI, entry, tests — all run unchanged on both.

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
│     ├─ flows/             ← high-level pre-built flows (boot, provisioning, OTA, ...)
│     ├─ entry/             ← one parameterized entry template
│     ├─ observe/           ← log, trace, error domains, counters
│     └─ test/              ← fakes, mocks, headless harness
└─ src/                     ← mirror
```

Directory renames are cheap compared to the meaning changes inside them. Most of the work is *what lives where*, not moving files.

---

## What Gets Deleted

Every refactor fails by adding without removing. This one deletes first.

- `app_ctx::services()` — the locator. Gone.
- `app_services` as a product-facing type — gone. Runtime-internal only, then deleted when B-phases complete.
- `map_intent` + `map_event` + `capability_event_discriminant` — gone. One `on_event`.
- `k_capability_event_discriminant_unset` — gone with the discriminant.
- `#ifdef ESP_PLATFORM` in any header above `hal/` — gone.
- `entry.hpp` macro variants (`INTERACTIVE`, `DASHBOARD`, `MAIN_HOST`, `MAIN_DEVICE`, …) — gone. One template.
- `shell` as an owner of chrome logic — gone. Passive presentation only.
- `screen_router` / `overlay_manager` / `focus_scope` as public types — gone. Private children of the navigation controller.
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

Ten phases. Each is one PR, each leaves `main` buildable. Targets are caps — miss one, reassess scope before merging.

### P0. Foundation — before any refactor touches behavior

**Purpose:** every later phase rides on these. Built first so the refactor never flies blind.

- **Observability primitives** under `framework/observe/`: structured log (`BLUSYS_LOG(level, domain, fmt, ...)`), error domains (one enum per subsystem, no stringly-typed errors), trace macros (compile-out on release), counters. Replace scattered `BLUSYS_LOGE` with the structured call.
- **Test harness** under `framework/test/`: headless reducer runner, HAL fake backend that compiles with host build, capability mocks. Host build becomes first-class — the fast iteration loop.
- **Lint guards** under `scripts/`:
  - `check-no-service-locator.sh` — greps for `ctx.services()`
  - `check-no-platform-ifdef-above-hal.sh` — bans `#ifdef ESP_PLATFORM` outside `hal/`
  - `check-public-api.sh` — bans non-`blusys.hpp` app-facing includes in examples
  - `check-capability-shape.sh` — enforces capability contract
- **CI wiring:** every guard runs on every PR. Host build + one device build gate merge.

**Targets:** test harness runs; all lint guards exist (may initially pass trivially — they activate as phases land).

**Deletes:** ad-hoc log patterns in at least one subsystem to prove the structured log works.

### P1. HAL hardening

**Purpose:** make the HAL a real contract, not an accidental one.

- Formalize the C ABI in `include/blusys/hal/`. Every peripheral has: types + functions + error domain. No macros leaking implementation.
- **Shared callback-lifecycle helper** under `hal/internal/callback_lifecycle.h` — the `callback_active` counter + `closing` flag + critical-section-guarded `try_enter` + `wait_for_idle` drain. Apply to `gpio.c`, `timer.c`, `uart.c`, `button.c`, `encoder.c`.
- **Host HAL backend** — fakes for every peripheral used by tests. Lives alongside the device backend. Selected by build system, not by `#ifdef` in source.

**Targets:** `gpio.c` + `timer.c` + `button.c` + `encoder.c` + `uart.c` lose ≥150 LOC combined. One drain implementation. Host fakes compile. `check-no-platform-ifdef-above-hal.sh` passes.

**Deletes:** all per-peripheral drain loops; all ad-hoc host stubs scattered through source.

### P2. Services internalized

**Purpose:** services become internal plumbing. Nothing in `framework/` or app code imports them directly.

- **Directory split:** `services/{net,session,storage}/`. Move files, update includes.
- **Storage helper** consolidating `build_path`, lock, CRUD, `blusys_translate_esp_err`. `fs.c` + `fatfs.c` become thin backend adapters.
- **Net bootstrap helper** (NVS + event loop + netif + singleton guard + shutdown ordering) for `wifi.c`, `wifi_prov.c`, `espnow.c`, `wifi_mesh.c`, `bluetooth.c`.
- **Session helper** (EventGroup + connected/closing + worker-task + timeout) for `mqtt.c`, `ws_client.c`. **Test prerequisite:** connect-success, connect-timeout, disconnect-while-inflight, reconnect tests exist and pass before the helper lands.
- **Access rule:** services are reachable only by capabilities (P5). Lint enforces it once P5 lands.

**Targets:** `fs.c` + `fatfs.c` ≤ 400 LOC combined. Net files lose ≥20% combined. Session files lose ≥25% combined.

**Deletes:** parallel CRUD wrappers; duplicate bootstrap scaffolding; duplicate session state machines.

### P3. Effect system — the architectural core

**Purpose:** replace `ctx.services()` with typed effects. This is the hinge of the whole refactor.

- Introduce `blusys::fx<App>` — a compile-time-assembled dispatcher with one method group per registered capability. Capabilities declare their effect namespace; the runtime wires them at init.
- Reducer signature becomes: `auto reduce(State, Action, fx&) -> State;` Effects are requested via `fx`, not returned from the reducer (reducer stays pure; `fx` is the side-effect interface).
- Capabilities implement two things: the effect handler (what happens when `fx.x.y(...)` is called) and the event emitter (what enters the event bus).
- Delete `app_services` from public headers. What remains internally is an implementation detail of `fx` assembly.
- Migrate every call site in examples and `src/framework/screens/` in the same PR. No shim.

**Targets:** zero `ctx.services()` call sites in product-facing code. `check-no-service-locator.sh` passes. Every capability exposes at most one effect namespace.

**Deletes:** `app_services`, `app_ctx::services()`, every `ctx.services().xxx()` call.

### P4. One event pipeline

**Purpose:** collapse the three-hook dispatch to one.

- Rename `framework/engine/` → `framework/events/`. One event type `blusys::event` with an intrinsic tag (source, kind, id, payload). Framework intents (button/encoder/touch) become event kinds on the same bus.
- Spec exposes one hook: `auto on_event(event, state) -> std::optional<Action>;` Runtime does all translation before the hook sees the event.
- Input subsystem becomes capabilities that emit events. No special-case input path.
- Delete `map_intent`, `map_event`, `capability_event_discriminant`, `k_capability_event_discriminant_unset`, `integration_dispatch.hpp`. Update `app_runtime.hpp:318-380` to a single dispatch function.

**Targets:** one dispatch function in `app_runtime`. Spec has one event hook. `framework/events/` has one public event type.

**Deletes:** the three spec hooks, the discriminant, the intent type as a distinct concept, `integration_dispatch`.

### P5. Capability shell + access contract

**Purpose:** capabilities are a real extension unit with a real contract, not a copy-pasted header pattern.

- **Shell:** CRTP base providing `kind_value` / `kind()` / `status()` / effect-handler registration / event emission. Each capability subclass declares only capability-specific members.
- **Platform split:** one header per capability, one `*_host.cpp` and one `*_device.cpp`. No `#ifdef ESP_PLATFORM` anywhere in the header.
- **Event map:** `capability_event_map.cpp` is the single registry for raw IDs. Capability headers reference it; nothing re-encodes numeric contracts.
- **Access contract:** capabilities are the only callers of `services/*`. Lint enforces the boundary.
- **Extension template:** a documented 50-line template for "adding a new capability" plus one canonical example capability. This is the contract.

**Targets:** each capability `*.hpp` ≤ 60 LOC of class shell. Zero `#ifdef ESP_PLATFORM` in capability headers. `check-capability-shape.sh` passes.

**Deletes:** duplicated class shells; inline `#ifdef` bodies; per-capability event-ID redeclarations.

### P6. UI as a reactive tree

**Purpose:** one owner for navigation, overlays, focus, chrome. The UI has a root.

- Introduce `framework/ui/controller/` — one `navigation_controller` that owns route stack, overlay stack, focus scope, and shell chrome. `screen_router`, `overlay_manager`, `focus_scope`, `shell` become private implementation details.
- One transition hook runs on every navigation event. Back-visibility, tab highlight, title sync happen there. Nowhere else.
- `fx.nav.*` effects dispatch through the controller. Product code never touches router/overlay/focus directly.
- `shell` becomes passive presentation with no chrome logic of its own.

**Targets:** zero direct access to `screen_router` / `overlay_manager` / `focus_scope` from product code (examples + capabilities). One transition hook; chrome updates occur only through it.

**Deletes:** direct access to UI internals; duplicate chrome sync paths; `shell`'s active chrome logic.

### P7. Entry consolidation

**Purpose:** one way to launch a blusys app.

- One template: `BLUSYS_APP_ENTRY(app_spec)` where `app_spec` carries platform init, variant choice, initial flow. Platform-specific init lives behind the spec, not inlined.
- All macro variants (`INTERACTIVE`, `DASHBOARD`, `MAIN_HOST`, `MAIN_DEVICE`, …) collapse to one. At most one extra macro if a legacy linker symbol genuinely requires it.
- Flows become first-class spec inputs. Picking a flow is a spec field, not a macro choice.

**Targets:** macro variant count ≤ 2. Zero platform init in macro bodies. All examples switch to the single template.

**Deletes:** every legacy entry macro; inline display/input init in entry.

### P8. Display flush cleanup

**Purpose:** local cleanup; the display↔lcd boundary is already correct.

- Extract named strategy helpers in `display.c` (mono-page pack, RGB565 swap, direct blit). `flush_cb()` body becomes a small dispatcher.
- Do not rearrange the `display.c` ↔ `lcd.c` boundary — `lcd.c` has no LVGL knowledge, `display.c` has no panel-setup code. That part is fine.

**Targets:** `flush_cb()` body ≤ 20 LOC. Each panel-kind path is one named helper.

**Deletes:** inline panel-kind branches inside `flush_cb`.

### P9. Public API lockdown + docs

**Purpose:** seal the surface so accretion doesn't start over.

- `blusys.hpp` re-exports exactly the app-author API. Anything not re-exported is internal.
- `check-public-api.sh` bans examples and external app code from including non-`blusys.hpp` headers.
- Docs rewritten to reflect the new model:
  - `docs/internals/architecture.md` — the four commitments, the directory shape.
  - `docs/app/app-runtime-model.md` — reducer + `fx` + events.
  - `docs/app/capability-composition.md` — the capability contract, template, canonical example.
  - `docs/app/adding-a-capability.md` — new; step-by-step for the only extension unit.
  - `docs/app/adding-a-flow.md` — new; flows as spec inputs.
- Delete docs describing the old split, old bridges, old macros.

**Targets:** `blusys.hpp` is the only public include used by examples. Docs match code exactly.

**Deletes:** every doc page describing pre-refactor state.

---

## Track Structure

```
P0 ── P1 ── P2 ─┐
                ├─ P3 ── P4 ── P5 ── P6 ── P7 ── P8 ── P9
             (P2 before P3 so effects can wire services)
```

- **P0–P2** are infrastructure. Low risk. Each is independently landable.
- **P3–P5** are the architectural core. P3 (effects) unblocks everything above it. P4 (events) can land in parallel with P5 if coordinated, but the serial order is safer.
- **P6–P7** are product-facing. Depend on P3 for `fx.nav`.
- **P8–P9** are sealing and cleanup.

Recommended: serialize. Parallelism costs more in coordination than it saves in wall time at this scope.

---

## Invariants enforced by CI after P9

These are the guarantees the refactor produces:

1. No `ctx.services()` anywhere in the repo.
2. No `#ifdef ESP_PLATFORM` outside `components/blusys/*/hal/` or backend `.cpp` files.
3. Examples include only `<blusys.hpp>`.
4. Every capability has exactly one header + one `*_host.cpp` + one `*_device.cpp`.
5. Every service file is reached only from `framework/capabilities/` or `framework/runtime/`.
6. Host build + one device build pass on every PR.
7. Layering, mirror, namespace, host-bridge, public-API, service-locator, platform-ifdef, capability-shape checks all pass.

If any invariant is violated, the violation — not an exception — is the fix.

---

## What I am not doing and why

- **Not rewriting the reducer pattern.** It's right for this class of platform. Typed effects + one event bus are the cleanup; the core idea stays.
- **Not switching away from LVGL.** Rendering is a solved problem here; changing it would be churn without benefit.
- **Not introducing coroutines / async.** Fixed-capacity, synchronous dispatch, explicit event bus — the whole platform is easier to reason about this way. Don't add magic.
- **Not building a plugin loader.** Capabilities are compile-time. Dynamic loading is a different product; don't pay for flexibility you don't use.
- **Not versioning the public API.** No users, no version. When users arrive, then version.

---

## Exit criteria

- All four architectural commitments hold in code, enforced by CI.
- All nine "What Gets Deleted" items are actually deleted.
- All phase LOC and count targets are met.
- Adding a new capability takes one header + one `*_host.cpp` + one `*_device.cpp` and nothing else.
- Host build runs the full reducer + capability test suite with fake HAL.
- `blusys.hpp` is the only public header an app author includes.
- Docs describe the current architecture, not the history.

This is the plan. Start at P0.
