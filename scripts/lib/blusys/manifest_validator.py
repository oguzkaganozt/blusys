from __future__ import annotations

import sys
from pathlib import Path
from typing import Any

try:
    import yaml
except ImportError:
    print("ERROR: PyYAML is required. Install with: pip install pyyaml")
    sys.exit(2)

REPO_ROOT = Path(__file__).resolve().parents[3]
MANIFEST_KEYS = ("schema", "interface", "capabilities", "profile", "policies")


def load_catalog(repo_root: Path = REPO_ROOT) -> dict:
    path = repo_root / "scripts" / "scaffold" / "catalog.yml"
    if not path.exists():
        raise SystemExit(f"ERROR: {path} not found")
    with open(path, encoding="utf-8") as f:
        catalog = yaml.safe_load(f)
    if not isinstance(catalog, dict):
        raise SystemExit(f"ERROR: {path} did not contain a mapping")
    return catalog


def _join_values(values: list[str]) -> str:
    return ", ".join(values)


def _validate_string_list(value: Any) -> bool:
    return isinstance(value, list) and all(type(item) is str for item in value)


def validate_manifest(manifest: Any, catalog: dict) -> list[str]:
    errors: list[str] = []
    if not isinstance(manifest, dict):
        return ["manifest must be a mapping"]

    allowed_keys = set(MANIFEST_KEYS)
    for key in manifest:
        if key not in allowed_keys:
            errors.append(f"unknown top-level key {key!r}")

    interfaces = catalog.get("interfaces", {})
    capabilities = catalog.get("capabilities", {})
    policies = catalog.get("policies", {})
    profiles = catalog.get("profiles", {})

    if "schema" not in manifest:
        errors.append("missing required field 'schema'")
    else:
        schema = manifest.get("schema")
        if type(schema) is not int:
            errors.append("schema must be an integer")
        else:
            supported = catalog.get("schema")
            if supported is None:
                errors.append("catalog is missing a schema version")
            elif schema != supported:
                errors.append(
                    f"unsupported schema {schema}; supported range: {supported}..{supported}"
                )

    interface = manifest.get("interface")
    if "interface" not in manifest:
        errors.append("missing required field 'interface'")
    elif type(interface) is not str:
        errors.append("interface must be a string")
    elif interface not in interfaces:
        errors.append(
            f"unknown interface {interface!r}; valid values: {_join_values(list(interfaces))}"
        )

    if "capabilities" in manifest:
        value = manifest.get("capabilities")
        if not _validate_string_list(value):
            errors.append("capabilities must be a list of strings")
        else:
            unknown = [cap for cap in value if cap not in capabilities]
            if unknown:
                errors.append(
                    f"unknown capabilities: {_join_values(list(dict.fromkeys(unknown)))}; "
                    f"valid values: {_join_values(list(capabilities))}"
                )

    if "profile" in manifest:
        value = manifest.get("profile")
        if value is not None and type(value) is not str:
            errors.append("profile must be a string or null")
        elif type(value) is str:
            if value not in profiles:
                errors.append(
                    f"unknown profile {value!r}; valid values: {_join_values(list(profiles))}"
                )
            else:
                supported_interfaces = profiles[value].get("interfaces", [])
                if (
                    supported_interfaces
                    and type(interface) is str
                    and interface in interfaces
                    and interface not in supported_interfaces
                ):
                    errors.append(
                        f"profile {value!r} does not support interface {interface!r}; "
                        f"valid interfaces: {_join_values(list(supported_interfaces))}"
                    )

    if "policies" in manifest:
        value = manifest.get("policies")
        if not _validate_string_list(value):
            errors.append("policies must be a list of strings")
        else:
            unknown = [policy for policy in value if policy not in policies]
            if unknown:
                errors.append(
                    f"unknown policies: {_join_values(list(dict.fromkeys(unknown)))}; "
                    f"valid values: {_join_values(list(policies))}"
                )

    return errors


def validate_manifest_text(text: str, catalog: dict) -> list[str]:
    try:
        manifest = yaml.safe_load(text)
    except yaml.YAMLError as exc:
        return [f"failed to parse manifest: {exc}"]
    return validate_manifest(manifest, catalog)
