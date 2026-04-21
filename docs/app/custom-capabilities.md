# Product-custom capabilities

Use this when a capability belongs to one product and should stay out of the shared catalog.

## Shape

```cpp
struct pulse_config {
    std::uint32_t interval_ms = 500;
    const char *label = "override-demo";
};

struct pulse_status : blusys::capability_status_base {
    bool running = false;
    std::uint32_t pulse_count = 0;
    std::uint32_t last_pulse_ms = 0;
};

enum class pulse_event : std::uint32_t {
    pulse = 0x0901,
    capability_ready = 0x09FF,
};

class pulse_capability final : public blusys::capability_base {
public:
    static constexpr blusys::capability_kind kind_value = blusys::capability_kind::custom;

    explicit pulse_capability(const pulse_config &cfg);

    [[nodiscard]] blusys::capability_kind kind() const override { return kind_value; }
    [[nodiscard]] const pulse_status &status() const { return status_; }

    blusys_err_t start(blusys::runtime &rt) override;
    void poll(std::uint32_t now_ms) override;
    void stop() override;

private:
    void post_event(pulse_event event);
};
```

- Keep the shell data-only and small.
- Use `capability_kind::custom` and the `0x0900` product-custom event band.
- Emit `capability_ready` from `start()`.
- Keep `poll()` non-blocking.
- In the reducer, bridge raw `event.id` values to actions and query `ctx.status_of<pulse_capability>()` when you need live state.

If the capability becomes reusable, promote it to a named framework capability and add it to the catalog.

## Reference example

- [`examples/reference/override/`](https://github.com/oguzkaganozt/blusys/tree/main/examples/reference/override/) shows a host-buildable product with a hand-written `app_spec` and one product-local capability.

## See also

- [Authoring a capability](capability-authoring.md) — framework capability contract
- [Adding a capability](adding-a-capability.md) — step-by-step framework capability shell
