# C++ Platform Policy

This document defines the Blusys platform's C++ language policy normatively. It is no longer a discussion note — these rules apply to all framework code in the repo.

For the full decision log see [`DECISIONS.md`](DECISIONS.md). For the broader application layer context see [`application-layer-architecture.md`](application-layer-architecture.md).

## Decision

The platform has three tiers. **Framework is the only C++ tier in V1.**

| Tier | Component | Language | Public headers |
|---|---|---|---|
| 1 | `components/blusys/` (HAL + drivers) | C | C (`extern "C"`) |
| 2 | `components/blusys_services/` | C | C (`extern "C"`) |
| 3 | `components/blusys_framework/` | C++ | C++ |

Dependency direction: `blusys_framework` → `blusys_services` → `blusys`. Reverse dependencies are forbidden.

Product app code under `app/` is C++. The bootstrap entry point is `main/app_main.cpp` (C++ with `extern "C" void app_main(void)`).

Services migration to C++ is **deferred to V2**. Services callback orchestration is valuable ground for C++ eventually, but the current `wifi.c` / `mqtt.c` implementations are clean and migrating them now would open a long hybrid C/C++ window for speculative benefit. Framework-only C++ in V1 delivers the concrete widget-kit value without the migration cost.

## Why these tiers

**HAL stays C** because it is the layer closest to the MCU. Explicit lifecycle, deterministic behavior, and the simplest possible low-level surface are most critical here. HAL wraps ESP-IDF drivers that are themselves C.

**Drivers stay C and live inside `components/blusys/`** because driver modules (display, input, sensor, actuator) compose HAL primitives into hardware drivers. They are mostly stateless or hold trivial state, and they do not benefit meaningfully from C++ encapsulation. Keeping them C avoids forcing C++ on modules that do not need it. They live inside the HAL component (not a separate component) to keep the platform at three tiers instead of four — the HAL/drivers boundary is enforced by directory discipline + CI lint (see [`DECISIONS.md`](DECISIONS.md) decisions 4 and 16).

**Services stays C** in V1. The current C services (`wifi.c`, `mqtt.c`, `ota.c`, ...) are already well-factored around opaque handles and explicit lifecycle — the same pattern C++ would give us with classes. Migrating them to C++ is a V2 concern; in V1 it would force every service-tier example from `.c` to `.cpp`, open a hybrid C/C++ window of undefined length, block LTO, and require a dual-header policy — all for gains that are speculative until a concrete pain point shows up.

**Framework is C++** because the widget kit needs designated initializers for config structs, captureless lambdas for semantic callbacks, `enum class` for variants, and template containers. Controller state machines, router logic, intent dispatch, and feedback channels all benefit from the same toolset. This is where C++ earns its keep concretely — not speculatively.

## Selected C++ model

**C with Classes.**

- `-std=c++20`
- `-fno-exceptions`
- `-fno-rtti`
- `-fno-threadsafe-statics`
- exceptions: not used
- RTTI: not used
- RAII over HAL handles or LVGL objects: not used
- constructor does no real work; lifecycle is explicit
- template usage is minimal
- function-local statics with non-trivial destructors: forbidden (consequence of `-fno-threadsafe-statics`)

The core principle: **architecture solves spaghetti, not language features**. This policy still applies; C++ is chosen only where it reduces friction, not to enable heavy OOP frameworks.

### Why C++20 and not C++17

The platform uses C++20 specifically because:

- **`std::span<T>`** (C++20) is part of the allowed container set — it expresses the "caller-owned buffer" allocation pattern cleanly, and is the natural replacement for `(ptr, len)` pairs.
- **Designated initializers** (C++20) are used throughout widget config struct examples — they make config call sites readable at a glance.
- **`constexpr` improvements** (C++20) make the theme-token and compile-time constant patterns simpler.

ESP-IDF 5.5 ships with GCC 14, which has full C++20 support for Xtensa and RISC-V targets. No new toolchain work is required.

C++17 was rejected because it would force us to either drop `std::span` and designated initializers (making the widget kit verbose), or document both as GCC extensions (informal, non-portable). C++20 is the honest choice for the feature set we want to use.

## Public ABI rules

- HAL headers: C, `extern "C"` guarded. No C++ types or idioms. Example: `components/blusys/include/blusys/gpio.h`.
- Driver headers: C, `extern "C"` guarded. Same rules as HAL headers. Example: `components/blusys/include/blusys/drivers/input/button.h`.
- Service headers: C, `extern "C"` guarded. Example: `components/blusys_services/include/blusys/connectivity/wifi.h`.
- Framework headers: C++. Namespaces, references, method-style APIs allowed. Example: `components/blusys_framework/include/blusys/framework/ui/widgets/button.hpp`. In V1, framework headers may still expose lightweight LVGL handle/value types such as `lv_obj_t*`, `lv_color_t`, `lv_font_t*`, and `lv_group_t*` where that keeps the API honest and thin. Raw LVGL event contracts (`lv_event_t*`, `lv_event_cb_t`) should stay behind the widget boundary.

Examples consuming only HAL, drivers, or services stay `.c` files. Examples consuming the framework are `.cpp` files with `extern "C" void app_main(void)` as the bootstrap.

The `blusys/...` include namespace prefix is preserved everywhere. Umbrella headers:
- `blusys/blusys.h` — HAL + drivers (C)
- `blusys/blusys_services.h` — services (C)
- `blusys/framework/framework.hpp` — framework (C++)
- `blusys/blusys_all.h` — removed at end of transition (see Phase 8)

## Compile flags

Default C++ build options for the framework component (framework is the only C++ component in V1):

```cmake
target_compile_options(${COMPONENT_LIB} PRIVATE
    -std=c++20
    -fno-exceptions
    -fno-rtti
    -fno-threadsafe-statics
    -Os
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
)
```

LTO (`-flto`) is available because only one component (framework) is C++; there is no hybrid C/C++ link-time uncertainty to work around. It can be enabled whenever the framework component ships its first widget.

## Forbidden and allowed features

### Forbidden

- `new`, `delete`, `std::make_unique`, `std::make_shared`
- `std::function`
- `std::string`, `std::vector`, `std::map`, `std::unordered_map`, `std::list`, `std::deque` as data members
- `dynamic_cast`, `typeid`
- `throw`, `try`, `catch`
- function-local `static` with non-trivial destructor
- `std::shared_ptr`, `std::weak_ptr`
- virtual inheritance graphs (single-level virtual is allowed where useful)
- `std::thread`, `std::mutex` (use FreeRTOS primitives via `blusys/internal/blusys_lock.h`)
- destructors that call `lv_obj_delete()` or free HAL handles (LVGL and HAL own those lifetimes)
- RAII wrappers that take ownership of LVGL objects or HAL handles

### Allowed

- namespaces
- `class`, `struct`, `private`, `public`, `protected`
- `enum class`
- `constexpr`, `consteval`, `inline`, `if constexpr`
- **designated initializers** (C++20) — used extensively in widget config structs
- single-level `virtual` for injection points where necessary (e.g., feedback sinks)
- templates where there is real reuse benefit; no deep template metaprogramming
- `std::array<T, N>`
- `std::span<T>` (C++20) — preferred for caller-owned buffers
- `std::optional<T>` (watch the size cost)
- `std::string_view`
- `std::variant<T...>` with tag-based access only (never `std::get_if` with RTTI assumptions)
- captureless lambdas (decayable to function pointers via the `+[]` idiom)
- `std::move`, `std::forward`

## Allocation policy

**Fixed-capacity containers + caller-owned buffers.** No dynamic allocation in steady state. Applies to framework C++ code; services (C) already follows the same pattern through opaque handles and explicit `calloc`/`free` at `init()`/`close()` time.

The platform ships a small container set in `blusys_framework/core/containers.h`:

- `blusys::array<T, N>` — fixed-size array with bounds-checked access
- `blusys::string<N>` — fixed-capacity string
- `blusys::ring_buffer<T, N>` — SPSC ring buffer
- `blusys::static_vector<T, N>` — fixed-capacity vector with push_back semantics

Rules:

1. No `new` or `delete` in framework code.
2. `calloc` / `free` are permitted only at module `init()` time to allocate long-lived structures; no allocation after init returns. Exception: caller-owned buffers explicitly passed in by the application.
3. Large or variable payloads (MQTT bodies, HTTP bodies, LCD framebuffers, file I/O buffers) are **caller-owned**. Modules accept `std::span<uint8_t>` or raw `(ptr, len)` pairs from the caller and never allocate them internally.
4. Pool allocators may be introduced only where bounded allocation is genuinely required and the budget is measured (not assumed). The Camp 2 widget slot pool (see [`UI-system.md`](UI-system.md)) is the canonical example: fixed-capacity, per-widget, fail-loud on exhaustion.
5. Small bounded state lives in `blusys::static_vector` / `blusys::string<N>` on the module's own handle struct.

This policy is enforced by code review, not by compile-time checks.

## Concurrency

C++ does not relax any thread-safety rule in this platform.

- FreeRTOS primitives only (via `blusys/internal/blusys_lock.h`).
- Never hold a `blusys_lock_t` across a blocking wait (`xEventGroupWaitBits`, `vTaskDelay`, network/disk wait, long service call). The pattern is: take lock → prepare → give lock → block. See `wifi.c:blusys_wifi_connect()` and `mqtt.c:blusys_mqtt_connect()` as reference implementations.
- Never hold `blusys_ui_lock` across a blocking wait. Same reasoning.
- Service callbacks must not directly mutate UI widgets or application state. They post events to the controller layer.
- Controllers own all state transitions.

## Controller model

The normative controller API is:

```cpp
struct controller {
    blusys_err_t init();
    void         handle(const app_event& e);
    void         tick(uint32_t now_ms);
    void         deinit();
};
```

- `init()` / `deinit()` are explicit (no constructor/destructor work).
- `handle()` processes discrete events from the framework (input intents, service callbacks routed through integrations).
- `tick()` is called by the framework at a configurable cadence (default: every 10 ms). Use it for polling, timeouts, time-based feedback, and animations tied to state.
- Controllers are small and domain-scoped. Avoid mega-controllers.

Controllers may:
- call simple, single-capability platform APIs directly (e.g., read battery level)
- produce router commands
- manage their own state

Controllers must push into `app/integrations/`:
- callback-heavy flows
- multi-capability composition
- product-specific platform adapters
- non-trivial wiring between services

For the router command set, intent enum, and feedback channel API, see [`application-layer-architecture.md`](application-layer-architecture.md).

## LVGL and widget kit note

The widget kit (`blusys_framework/ui/`) is C++, uses LVGL as its engine, and follows the widget architecture defined in [`UI-system.md`](UI-system.md). Key rules repeated here because they affect C++ style:

- `lv_obj_t*` ownership stays with LVGL. Never put `lv_obj_delete()` in a destructor.
- V1 reduces LVGL exposure in normal product code, but does not attempt to hide LVGL completely. Opaque handles and engine-native value types are acceptable in public framework headers; raw LVGL event APIs are not.
- Widget state is managed via explicit setters (`<name>_set_*`), not RAII.
- Draw callbacks read from `blusys::ui::theme()` — no hardcoded colors or pixels.
- Custom widget classes use LVGL's `lv_obj_class_t` pattern directly; no C++ inheritance over LVGL types.

## Preferred style

- namespaces over free functions in global scope
- small classes with explicit lifecycle
- composition over inheritance
- free functions when a class is not justified
- `enum class` for all state modeling
- `constexpr` for tokens and compile-time constants
- trailing return types only when they improve readability

## Avoided style

- class-heavy framework chains
- inheritance-and-virtual graphs
- template metaprogramming beyond light generic containers
- RAII over external resource handles
- `std::shared_ptr` / reference counting
- singletons implemented via Meyers' singleton pattern (blocked by `-fno-threadsafe-statics`)

## Rejected alternatives

### All C, including framework
Rejected. The widget kit's design (designated-initializer config structs, captureless-lambda callbacks, `enum class` variants, templated fixed-capacity containers) is where C++ earns its keep concretely. Keeping framework in C would either force verbose alternatives for every one of those features or rely on GCC extensions.

### All C++, including HAL, drivers, and services
Rejected. HAL must stay the simplest, most deterministic layer possible. Drivers compose HAL into hardware drivers with trivial state; C++ would add overhead for no real gain. Services are already well-factored in C around opaque handles and explicit lifecycle — the same pattern C++ would give us with classes — and the `wifi.c` / `mqtt.c` implementations are held up in CLAUDE.md as correct reference implementations. Migrating them buys no concrete value in V1 and opens a long hybrid C/C++ window.

### Full C++ with exceptions and RTTI
Rejected. Exceptions and RTTI cost flash and RAM on embedded targets, interact poorly with ESP-IDF system code, and introduce non-deterministic control flow.

### Full RAII over HAL and LVGL handles
Rejected. LVGL's parent-child widget lifecycle cannot be safely wrapped by C++ destructors; HAL handles need explicit close semantics for correct hardware state. RAII here creates double-free and ordering bugs.

### Services C++ in V1 (originally planned)
Rejected during the pre-implementation refinement pass. The original plan migrated `wifi`, `mqtt`, and the `ui` runtime to C++ as a Phase 5c pilot. On review, the cost (hybrid C/C++ window, dual-header policy, LTO deferral, ~20 example conversions) outweighed the benefit (namespaces and method-style APIs on already-clean C code). Framework consuming C services across a language boundary is trivial — C++ calls C with no plumbing — so keeping services C costs the framework nothing. If services grow in complexity later, per-module migration to C++ is an option in V2. See [`DECISIONS.md`](DECISIONS.md) decision 1.

## Summary

- HAL, drivers, and services: C. Headers are `extern "C"` guarded.
- Framework: C++ with Classes model, exceptions/RTTI off. The only C++ tier in V1.
- Fixed-capacity allocation, caller-owned buffers, no dynamic allocation in steady state.
- Thread safety: FreeRTOS primitives, never hold locks across blocking waits.
- LVGL and HAL handles owned by their runtime, not by C++ destructors.
- Controllers: lightweight, domain-scoped, `init/handle/tick/deinit`.
- Logging via `blusys/log.h` wrapper in framework code (see [`DECISIONS.md`](DECISIONS.md) decision 14).

Refer to [`DECISIONS.md`](DECISIONS.md) for the full decision log and [`application-layer-architecture.md`](application-layer-architecture.md) for the broader platform architecture.
