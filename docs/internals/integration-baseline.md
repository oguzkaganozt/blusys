# Integration baseline (metrics)

Use this page to record **before/after** snapshots when shrinking `main/integration/` or adding framework-owned flows.

## How to measure

From the repo root:

```bash
python3 scripts/measure_integration_baseline.py
```

The script prints **lines of code** in `main/integration/` and counts **`map_event`** / **`map_intent`** function bodies (heuristic) for reference examples used in the V2 roadmap.

Re-run after major refactors and paste the output (with git commit hash) below.

## Manual notes

| Field | connected_headless | connected_device | gateway |
|-------|-------------------|------------------|---------|
| Commit (baseline) | | | |
| `main/integration/` LOC | | | |
| `map_event` / `map_intent` branches (approx.) | | | |
| Action queue capacity (template arg if overridden) | default 16 | | |
| Notes | | | |

## Queue depth

The default action queue capacity is **`app_runtime<State, Action, 16>`** (16) unless a project uses a custom third template argument. See [App runtime model](../app/app-runtime-model.md) for overflow behavior and **`action_queue_drop_count()`**.
