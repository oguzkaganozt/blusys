# Blusys

Internal ESP32 product platform on ESP-IDF v5.5+.

Blusys gives product teams one app model, one scaffold, and one path from host-first iteration to hardware.

## The model

- `interface`: `interactive` or `headless`
- `capabilities`: connectivity, storage, OTA, diagnostics, and more
- `profile`: interactive hardware preset or `null`
- `policies`: overlays such as `low_power`

## Start here

1. [Getting started](docs/start/index.md)
2. [Product shape](docs/start/product-shape.md)
3. [App](docs/app/index.md)
4. [Examples](examples/README.md)

## Quick start

```sh
git clone https://github.com/oguzkaganozt/blusys.git ~/.blusys
~/.blusys/install.sh

mkdir ~/my_product && cd ~/my_product
blusys create
blusys host-build
```

Use `blusys create --list` to inspect the live catalog. Run `blusys create --interface headless` for connected products without local UI.

## What lives where

| Layer | Role |
|-------|------|
| [App](docs/app/index.md) | reducer, views, capabilities, profiles |
| [Services](docs/services/index.md) | runtime modules when you need exact lifecycle |
| [HAL + Drivers](docs/hal/index.md) | direct hardware access |
| [Internals](docs/internals/index.md) | architecture, checks, and testing |

## Validation

- `blusys validate` for repo preflight
- `blusys build-inventory <target> <ci_pr|ci_nightly>` for inventory-driven example builds
- `blusys host-build` for host iteration

## Docs

- Published site: <https://oguzkaganozt.github.io/blusys/>
- API reference: <https://oguzkaganozt.github.io/blusys/api/>

## Version

Current package version: `7.0.0`

## License

See [LICENSE](LICENSE).
