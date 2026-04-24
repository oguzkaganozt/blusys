# Contributing

Use the smallest set of checks that matches the scope of your change.

## Local checks

| Touches | Run |
|---------|-----|
| `components/` | `./blusys lint` |
| UI source paths | `python3 scripts/check-framework-ui-sources.py` |
| `inventory.yml`, new modules/examples/docs | `python3 scripts/check-inventory.py` |
| Product-shaped `main/` layouts | `python3 scripts/check-product-layout.py` |
| Example paths | `python3 scripts/example-health.py` |
| Scaffold or create matrix | `scripts/scaffold-smoke.sh` |
| Docs or `mkdocs.yml` | `mkdocs build --strict` |
| Components or examples | `blusys build` / `blusys host-build` |

## Host tests

```sh
ctest
```

For sanitizer coverage:

```sh
cmake -S scripts/host -B scripts/host/build-host-asan -DBLUSYS_HOST_SANITIZE=ON
cmake --build scripts/host/build-host-asan
ctest --test-dir scripts/host/build-host-asan
```

## Device and QEMU

```sh
blusys build <project> <esp32|esp32c3|esp32s3>
blusys qemu --serial-only <project> <qemu32|qemu32c3|qemu32s3>
```

After app-flow or integration changes, prioritize host interactive, headless, scaffold, and ST7735-class device builds on all three silicon targets.

## CI

PR CI runs layering checks, inventory checks, host smokes, docs strict build, inventory-selected device builds, display variants, and QEMU smoke subsets.

## Adding code

1. Add the code or content.
2. Register it in `inventory.yml`.
3. Add the nav entry in `mkdocs.yml` for docs.
4. Run the matching checks above.

## Release

Before tagging:

- `./blusys lint`
- `python3 scripts/check-inventory.py`
- `mkdocs build --strict`
- representative `blusys build` across `esp32`, `esp32c3`, and `esp32s3`
- `BLUSYS_VERSION_STRING` matches the tag

## See also

- [Architecture](architecture.md)
- [Guidelines](guidelines.md)
