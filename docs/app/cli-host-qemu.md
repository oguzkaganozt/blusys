---
title: Device, host & QEMU CLI
---

# Device, host, and QEMU CLI

Use `blusys build`, `blusys host-build`, and `blusys qemu` from the project root.

## Build targets

| Target | Output |
|--------|--------|
| `esp32` / `esp32c3` / `esp32s3` | `build-esp32*` |
| `host` | `build-host` |
| `qemu32` / `qemu32c3` / `qemu32s3` | `build-qemu*` |

## Common commands

| Goal | Command |
|------|---------|
| Build for hardware | `blusys build . esp32s3` |
| Build for host | `blusys host-build` |
| Build for QEMU | `blusys build . qemu32s3` |
| Run QEMU | `blusys qemu . qemu32s3` |
| Run QEMU framebuffer | `blusys qemu . qemu32s3 --graphics` |
| Serial-only QEMU | `blusys qemu . qemu32s3 --serial-only` |

`flash`, `run`, `monitor`, and `erase` accept device targets only.

## QEMU modes

- Default mode uses `qemu-system-* -nographic` for logs and CI-style runs.
- `--graphics` uses the IDF framebuffer path when the platform supports it.
- `--serial-only` skips the SDL window and keeps the monitor.

## Related

- [Validation and host loop](validation-host-loop.md)
- `scripts/host/README.md`
