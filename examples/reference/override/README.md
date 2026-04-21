# Override reference (blusys)

Shows the supported override-in-code path: a hand-written `app_spec` plus a product-local capability that stays out of the framework catalog.

## Build

```sh
blusys host-build examples/reference/override
blusys build examples/reference/override esp32s3
```

## Layout

- `main/platform/app_main.cpp` - explicit `app_main()` with the product-specific init zone
- `main/platform/override_capability.hpp` / `.cpp` - one-off product capability in the `0x0900` band
- `host/CMakeLists.txt` - host harness for the same sources

The host binary logs capability readiness and pulse events; stop it with `Ctrl-C` after a few ticks.

## See also

- [`docs/app/custom-capabilities.md`](../../../docs/app/custom-capabilities.md) - the product-custom capability shell
