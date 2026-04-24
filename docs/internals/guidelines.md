# Guidelines

Start with [Architecture](architecture.md). Use this page for API shape, product layout, and docs standards.

## API design

- HAL, drivers, and services use `blusys_` C APIs with `blusys_err_t` results.
- Product-facing APIs may use `blusys::...`.
- Keep public names direct and the common path short.
- Use opaque handles for stateful peripherals and simple scalars for simple calls.

## Language and logging

- HAL, drivers, and services expose C headers with `extern "C"` guards.
- Framework is the only C++ tier.
- Framework C++ uses `-std=c++20 -fno-exceptions -fno-rtti -fno-threadsafe-statics`.
- HAL and services use `esp_log.h`; framework uses `blusys/log.h`.
- Keep cross-tier boundaries explicit.

## Allocation and stability

- Framework code does not call `new`, `delete`, `malloc`, or `free` after init.
- Widget callback slots use fixed-capacity pools and fail loud on exhaustion.
- Do not expose target-specific behavior unless all supported targets share it.
- Do not recommend a public API without examples, docs, and validation.

## Product application shape

- Canonical `main/` is `app_main.cpp` plus optional `ui/`.
- The reducer is the center: `update(ctx, state, action)` owns domain rules.
- Use strong route and overlay enums.
- Prefer `ctx.fx()` for navigation and overlays.
- Keep view sync in bindings and small composites.

### Product application C++ shape {#product-application-cpp-shape}

- `ctx.product_state<YourState>()` in screen factories instead of file-scope state.
- Keep route handles in per-route structs and clear them on hide.
- Give interactive settings rows a stable `id` when using `on_changed`.
- Use `blusys/framework/app/` headers when you need typed capability event IDs or `dispatch_variant`.
- Use `blusys/framework/platform/build.hpp` or `reference_build.hpp` to avoid duplicated `#ifdef` matrices.

## Workflow

1. Confirm scope against `README.md` and this page.
2. Decide whether it is canonical product path, advanced path, or validation-only.
3. Define the public API shape and ownership boundary.
4. Define lifecycle and thread-safety rules.
5. Implement the smallest correct change.
6. Add only the docs, examples, and validation the change needs.
7. Validate the affected paths proportionally.

## Documentation standards

- Write for application developers first.
- Explain the task before listing APIs.
- Prefer short examples over long theory.
- Avoid repeating setup, build, and flash instructions.
- Keep user docs task-first and module docs reference-first.

### Module reference shape

1. Purpose
2. Supported targets
3. Quick example
4. Lifecycle
5. Thread safety
6. Limitations
7. See also

### Admonitions

- `!!! note` for recommended paths.
- `!!! tip` for a happy-path shortcut.
- `!!! warning` for invariants and breaking behavior.
- `???+` for long optional detail blocks.

## Module done criteria

Canonical product-path features should usually have:

- public API
- implementation
- at least one canonical example or quickstart
- user-facing documentation
- successful builds on the supported targets
- validation appropriate to the support tier

## Maintaining

| Directory | Component | Role |
|-----------|-----------|------|
| `components/blusys/` | `blusys` | all four layers, public headers under `include/blusys/` |

## Pre-merge checks

- `./blusys lint`
- `python3 scripts/check-inventory.py`
- `mkdocs build --strict`
- `blusys host-build`
- representative `blusys build` on changed component paths

For broader changes, also run `python3 scripts/check-product-layout.py` and `scripts/scaffold-smoke.sh`.
