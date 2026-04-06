# Plan: Simplify and Beautify Docs Architecture

## Context

The Blusys HAL documentation (MkDocs Material) works but feels plain and hard to navigate. 35 guides and 30 module pages sit in flat lists with no grouping. The Material theme is barely customized (no tabs, no dark mode, no icons, no cards). Module API reference pages use two inconsistent styles — older modules are narrative, newer modules have full type/function signatures. Several useful docs (architecture, vision, api-design-rules, testing-strategy, development-workflow, documentation-standards) are hidden from navigation entirely.

**Goal:** Make the docs easier to navigate, visually appealing, and internally consistent — without changing the actual technical content of the guides (which are already well-written).

---

## Phase 1: mkdocs.yml Overhaul

**File:** `mkdocs.yml`

Rewrite theme config to enable Material features:

- **Navigation tabs** (`navigation.tabs`, `navigation.tabs.sticky`) — 4 top-level tabs: Home, Guides, API Reference, Project
- **Dark/light toggle** — indigo/teal palette with scheme switcher
- **Section index pages** (`navigation.indexes`) — allows `guides/index.md` and `modules/index.md` as landing pages
- **Back-to-top** (`navigation.top`), **breadcrumbs** (`navigation.path`)
- **Search enhancements** (`search.highlight`, `search.suggest`)
- **Remove** `navigation.expand` (defeats the purpose of grouping)
- **Logo icon** — `material/chip`

Add markdown extensions:
- `pymdownx.details` + `pymdownx.superfences` — collapsible admonitions, better code
- `pymdownx.tabbed` (alternate_style) — tabbed content blocks
- `pymdownx.highlight` + `pymdownx.inlinehilite` — better syntax highlighting
- `pymdownx.emoji` — Material icons in content
- `md_in_html` — needed for card grids
- `def_list` — definition lists

New nav structure with **categorized sections**:

```
Home:
  - index.md
  - Getting Started
  - Compatibility
Guides:
  - guides/index.md  (NEW - card grid landing)
  - Core Peripherals: GPIO, UART (Blocking), UART (Async), I2C Master/Slave, SPI Master/Slave
  - Analog: ADC, DAC, SDM, PWM
  - Timers & Counters: Timer, PCNT, RMT TX/RX, MCPWM
  - Bus: TWAI, I2S TX/RX, SD/MMC
  - Sensors: Touch, Temperature
  - System: System Info, NVS, Sleep, Watchdog
  - Networking: WiFi, HTTP Client, HTTP Server, MQTT, OTA, SNTP, mDNS
  - Testing: Hardware Smoke Tests, Concurrency Tests, Validation Template
API Reference:
  - modules/index.md  (NEW - card grid landing)
  - (same categories as Guides)
Project:
  - Architecture, Vision, API Design Rules, Contributing,
    Development Workflow, Testing Strategy, Documentation Standards, Roadmap
```

No new pip dependencies needed — all features ship with `mkdocs-material>=9.5`.

---

## Phase 2: New Index/Landing Pages

### 2a. `docs/index.md` — Redesign

Replace the plain bullet-list landing page with:
- Short hero description
- Material card grid with 3 cards: "Get Started", "Task Guides", "API Reference"
- Supported targets as a clean table
- Project status as an info admonition
- Proper relative links (not raw file paths)

### 2b. `docs/guides/index.md` — NEW

Card grid page linking to each peripheral category with Material icons.

### 2c. `docs/modules/index.md` — NEW

Card grid page for API reference categories, with a note about the reference format convention.

---

## Phase 3: Module Reference Standardization

**22 old-style module pages** need migration to match the newer format (wifi.md, mqtt.md, etc. as reference template).

Target format for every module page:
```
# Module Name
One-line purpose.

## Target Support
| Target | Supported |  (table, not bullet list)

## Types
### `blusys_<module>_config_t`  (full typedef code block)

## Functions
### `blusys_<module>_open`
  signature code block
  description
  **Parameters:** bullet list
  **Returns:** error codes

## Lifecycle
## Thread Safety
## ISR Notes (if applicable)
## Limitations  (merge old "Error Behavior" into here)
## Example App
```

For each of the 22 modules:
1. Read the public header from `components/blusys/include/blusys/<module>.h` to get accurate type definitions and function signatures
2. Restructure the page — convert "Supported Targets" bullet list to table, add Types section, convert "Blocking/Async APIs" to full Functions section, merge Error Behavior into Limitations
3. Preserve all existing Thread Safety, ISR Notes, Limitations content

**Modules to update:**
`system`, `gpio`, `uart`, `i2c`, `i2c_slave`, `i2s`, `spi`, `spi_slave`, `pwm`, `adc`, `dac`, `sdm`, `mcpwm`, `timer`, `pcnt`, `rmt`, `twai`, `touch`, `sdmmc`, `temp_sensor`, `wdt`, `sleep`

**Reference modules (already in target format, no changes):**
`wifi`, `nvs`, `http_client`, `http_server`, `mqtt`, `ota`, `sntp`, `mdns`

---

## Phase 4: Cross-Linking

- Add an **"API Reference" link** at the bottom of each guide page pointing to the corresponding module ref
- Add a **tip admonition** at the top of each module page pointing to the corresponding guide(s):
  ```
  !!! tip "Task Guide"
      For a step-by-step walkthrough, see [GPIO Basics](../guides/gpio-basic.md).
  ```
- Update `contributing.md` to reference the Project tab

---

## Phase 5: Verification

- Run `mkdocs build --strict` — must pass with zero warnings
- Spot-check rendered site with `mkdocs serve`:
  - Tabs render correctly
  - Dark/light toggle works
  - Card grids display on index pages
  - Category grouping appears in sidebar
  - Cross-links between guides and module refs work
  - All previously hidden docs are accessible under Project tab

---

## Critical Files

| File | Action |
|------|--------|
| `mkdocs.yml` | Complete rewrite |
| `docs/index.md` | Redesign with cards |
| `docs/guides/index.md` | NEW — card grid landing |
| `docs/modules/index.md` | NEW — card grid landing |
| `docs/modules/*.md` (22 files) | Standardize format |
| `docs/guides/*.md` (35 files) | Add API Reference link at bottom |
| `docs/modules/*.md` (30 files) | Add tip admonition at top |
| `docs/contributing.md` | Update references |
| `components/blusys/include/blusys/*.h` | READ ONLY — source for type/function signatures |

## Implementation Order

1. `mkdocs.yml` + new index pages (Phase 1+2) — get the structure right first
2. Module standardization (Phase 3) — biggest chunk of work, do in batches
3. Cross-linking (Phase 4) — quick pass over all files
4. `mkdocs build --strict` verification (Phase 5)
