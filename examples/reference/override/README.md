# Override reference (blusys)

Shows the supported override-in-code path: a hand-written `app_spec`, a product-local capability that stays out of the framework catalog, and a manifest so the shape still validates.

## Build

```sh
blusys host-build examples/reference/override
blusys build examples/reference/override esp32s3
```

## Layout

- `main/app_main.cpp` - explicit `app_main()` with the product-specific init zone
- `main/override_capability.hpp` / `.cpp` - one-off product capability in the `0x0900` band
- `host/CMakeLists.txt` - host harness for the same sources
- `blusys.project.yml` - manifest used for shape validation

The host binary logs capability readiness and pulse events; stop it with `Ctrl-C` after a few ticks.

## Validate

```sh
blusys validate examples/reference/override
```

## See also

- [`docs/app/custom-capabilities.md`](../../../docs/app/custom-capabilities.md) - the product-custom capability shell
