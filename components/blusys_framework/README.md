# blusys_framework

This component is the product-platform tier that sits above `blusys_services` and `blusys`.

Current structure:

- `src/core/` and `include/blusys/framework/core/` for product-lifecycle infrastructure
- `src/ui/` and `include/blusys/framework/ui/` for optional UI-layer code gated by `BLUSYS_BUILD_UI`

Rules:

- framework is the only C++ tier in V1
- use `blusys::framework` and related namespaces for public framework code
- keep constructors lightweight and lifecycle explicit
- no exceptions, no RTTI, no dynamic allocation in steady state

`BLUSYS_BUILD_UI` is expected to be provided by the top-level product build. When it is `ON`, UI-layer sources are added in this component's `CMakeLists.txt`; when it is `OFF`, only core sources build.
