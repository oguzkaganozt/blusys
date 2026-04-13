# Device, host, and QEMU (`blusys` CLI)

Single entry point for firmware, PC iteration, and ESP-IDF QEMU. Paths below are from your **project root** (where `CMakeLists.txt` lives).

## Build output directories

| Label | Directory | Role |
|-------|-----------|------|
| `esp32`, `esp32c3`, `esp32s3` | `build-esp32*` | Flash to hardware |
| `host` | `build-host` | SDL2 + LVGL host binary (no IDF flash image) |
| `qemu32`, `qemu32c3`, `qemu32s3` | `build-qemu*` | IDF firmware for QEMU; **separate** from chip dir so `sdkconfig` does not collide |

## Command matrix

| Goal | Command |
|------|---------|
| Firmware for a chip | `blusys build . esp32s3` (or `esp32` / `esp32c3`) |
| Host (SDL, same repo layout) | `blusys build . host` or `blusys host-build` |
| Firmware for QEMU (same Kconfig as chip unless you override) | `blusys build . qemu32s3` |
| Optional QEMU-only Kconfig merge | Edit project `sdkconfig.qemu` (scaffold ships a commented stub; full example: `examples/reference/interactive_dashboard/sdkconfig.qemu`) |
| Run QEMU after a build | `blusys qemu . qemu32s3` (UART / automation-style) |
| QEMU with IDF virtual framebuffer (LVGL / `esp_lcd_qemu_rgb` class apps) | `blusys qemu . qemu32s3 --graphics` |
| QEMU, IDF monitor, no SDL window | `blusys qemu . qemu32s3 --serial-only` |

`blusys flash`, `run`, `monitor`, and `erase` accept **device targets only** (`esp32` family), not `host` or `qemu*`.

## Two QEMU launch paths

`blusys qemu` **without** `--graphics` / `--serial-only` builds, merges a flash image, and runs **`qemu-system-*` with `-nographic`** — good for logs and CI-style checks.

With **`--graphics`** or **`--serial-only`**, the same build uses ESP-IDF’s **`idf.py qemu`** (SDL framebuffer where IDF supports it, or serial-only monitor). Interactive RGB-style demos usually need **`--graphics`** plus a suitable `sdkconfig.qemu` and HAL choice; see the interactive dashboard example.

## Related

- [Validation and host loop](validation-host-loop.md) — host smokes, scaffold, CI
- `scripts/host/README.md` — monorepo host harness (`blusys host-build` with no path)
- `examples/reference/interactive_dashboard/` — QEMU RGB + `sdkconfig.qemu` reference
