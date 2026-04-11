# Phase 6 archetype briefs (locked scope)

Execution-ready briefs for the two connected archetypes. Use with `PRD.md`, `ROADMAP.md` Phase 6, and `phase-6-connected-archetypes-plan.md`.

## Shared assumptions

- Runtime modes: `interactive` and `headless` only; archetypes are compositions.
- Structure: `main/` single component; `core/` / `ui/` / `integration/` ownership as in PRD.
- All capabilities: `integration/` wiring; events → `map_event` → reducer; no raw services in `core/` or `ui/`.

## Edge node (primary) — `examples/quickstart/edge_node/`

| Area | Decision |
|------|----------|
| **Purpose** | Field-style connected sensor/telemetry device: provision, connect, report, tolerate loss, OTA, expose health. |
| **Default entry** | Headless (`BLUSYS_APP_MAIN_HEADLESS`). Optional SSD1306 mono surface via `CONFIG_BLUSYS_EDGE_NODE_LOCAL_UI`. |
| **Operational phases** | `idle` → `provisioning` → `connecting` → `connected` → `reporting` (steady) → `updating` (OTA) → `error` as needed. Implemented in `core/app_logic.cpp` (`compute_phase` / `refresh_phase`). |
| **Reducer actions** | Full connectivity, telemetry, diagnostics, OTA, provisioning, storage, and periodic `sample_tick` tags — see `action_tag` in `core/app_logic.hpp`. |
| **Capabilities** | connectivity, telemetry, diagnostics, OTA, provisioning, storage — see `integration/app_main.cpp`. |
| **Telemetry** | Simulated temperature/humidity; `telemetry.record` in `on_tick` when phase is `reporting`; delivery callback logs batches (product would use MQTT/HTTP). |
| **Local control** | JSON status on device builds (`product`, `archetype`, `firmware`, `surface`). |
| **Recovery** | Wi-Fi loss clears IP; reducer logs buffering intent; telemetry pauses until `reporting` again. OTA failure clears in-progress flag and resumes. |
| **Host** | `host/CMakeLists.txt` — headless host binary; provisioning stub marks already provisioned. |

## Gateway / controller (secondary) — `examples/reference/gateway/`

| Area | Decision |
|------|----------|
| **Purpose** | Coordinator / operator hub: upstream connectivity, aggregated “downstream” telemetry simulation, diagnostics, OTA, optional local dashboard. |
| **Default entry** | Interactive on device (`BLUSYS_APP_MAIN_DEVICE`); host uses `BLUSYS_APP_MAIN_HOST_PROFILE` with SDL. |
| **Operational phases** | `idle`, `provisioning`, `connecting`, `connected`, `operational` (steady), `updating`, `error` — see `gateway::op_state`. |
| **UI** | Shell + dashboard + status screen (`ui/app_ui.cpp`); `operational_light` theme; ILI9341 / ILI9488 on device (Kconfig). |
| **Capabilities** | Same set as edge node; flush thresholds differ slightly to stress aggregation path. |
| **Orchestration** | `active_devices`, `agg_throughput` simulated in `on_tick`; not a second framework — plain reducer state. |
| **Local control** | JSON: `product`, `archetype`, `firmware`, `profile`. |

## Validation pointers

- Reducer phase logic: `scripts/host/src/operational_phase_smoke.cpp` mirrors edge node `compute_phase`.
- CI: PR builds inventory `ci_pr: true` examples including `edge_node`; host job builds and runs smoke binaries (see `.github/workflows/ci.yml`).
