# External shape

Validation sample that includes only `<blusys/blusys.hpp>` from app code.

## Build

```bash
blusys host-build examples/external
blusys build examples/external esp32s3
```

## Layout

- `main/platform/app_main.cpp` - minimal headless `blusys::app` entry using the umbrella header only
- `host/CMakeLists.txt` - host harness for the same source file
