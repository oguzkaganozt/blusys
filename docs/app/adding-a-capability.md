# Adding a capability

A capability is the only extension unit in Blusys. Adding a feature = one header + one `*_host.cpp` + one `*_device.cpp`. Nothing else changes.

## 1. Reserve an event-ID range

Open `components/blusys/include/blusys/framework/capabilities/capability.hpp` and add your kind to `capability_kind` plus a comment reserving the next free 256-wide event-ID block (the `0x0900` block is reserved for product-custom capabilities).

## 2. Write the header

Three parts: a `<name>_config`, a `<name>_status`, and the capability class.

```cpp
// framework/capabilities/example_sensor.hpp
#pragma once
#include <blusys/blusys.hpp>

namespace blusys {

struct example_sensor_config { uint32_t sample_interval_ms = 1000; };
struct example_sensor_status { bool running = false; int32_t last_value = 0; };

enum class example_sensor_event : uint32_t {
    sample_ready    = 0x0900,
    capability_ready = 0x09FF,
};

class example_sensor_capability : public capability_base {
public:
    static constexpr capability_kind kind_value = capability_kind::example_sensor;

    explicit example_sensor_capability(const example_sensor_config &cfg);

    capability_kind       kind()   const override { return kind_value; }
    example_sensor_status status() const          { return status_; }

    blusys_err_t start(runtime &) override;
    void         poll(uint32_t now_ms) override;
    void         stop() override;

private:
    example_sensor_config cfg_;
    example_sensor_status status_{};
    void post_event(example_sensor_event e);
};

}  // namespace blusys
```

Constraints: header ≤ 60 LOC of class shell. No `#ifdef ESP_PLATFORM` in the header.

## 3. Write the device implementation (`*_device.cpp`)

Real hardware work: ESP-IDF API, ISR registration, task creation. Gate all of it inside this file; never in the header.

```cpp
// src/framework/capabilities/example_sensor_device.cpp
#include "blusys/framework/capabilities/example_sensor.hpp"
// ... ESP-IDF includes ...

blusys_err_t blusys::example_sensor_capability::start(runtime &rt) {
    // hw init, set status_.running = true
    post_integration_event(static_cast<uint32_t>(example_sensor_event::capability_ready));
    return BLUSYS_OK;
}
```

## 4. Write the host stub (`*_host.cpp`)

Must compile without any hardware headers and emit `capability_ready` on first `poll()` so host builds reach steady state.

```cpp
// src/framework/capabilities/example_sensor_host.cpp
#include "blusys/framework/capabilities/example_sensor.hpp"

blusys_err_t blusys::example_sensor_capability::start(runtime &rt) {
    status_.running = true;
    post_integration_event(static_cast<uint32_t>(example_sensor_event::capability_ready));
    return BLUSYS_OK;
}

void blusys::example_sensor_capability::poll(uint32_t) {}
void blusys::example_sensor_capability::stop()         { status_.running = false; }
```

## 5. Register in the app spec

In `main/app_main.cpp`, add the capability to the spec's capability list and handle its events in `on_event`:

```cpp
static blusys::example_sensor_capability kSensor{{}};

static blusys::capability_list kCaps[] = {&kSensor};

spec.capabilities = kCaps;
spec.on_event = [](blusys::event e, State &s) -> std::optional<Action> {
    if (e.id == static_cast<uint32_t>(blusys::example_sensor_event::sample_ready))
        return Action::SensorSample{kSensor.status().last_value};
    return std::nullopt;
};
```

## 6. Verify

- Host build compiles and emits `capability_ready` without hardware.
- Add a validation example under `examples/validation/` that composes only your capability and asserts the event sequence.
- Run `scripts/check-capability-shape.sh`.

## See also

- [Authoring a capability](capability-authoring.md) — full contract reference (event-ID ranges, reserved fields)
- [Capability composition](capability-composition.md) — wiring multiple capabilities together
- [App Runtime Model](app-runtime-model.md) — how events reach the reducer
