#!/usr/bin/env bash
# check-capability-shape.sh
#
# Every capability is one header + one *_host.cpp + one *_device.cpp.
# Any new capability that diverges from that shape fails. The "rename-debt"
# metric tracks capability-header names whose `*_device.cpp` does NOT exist
# but where a legacy `<name>.cpp` does — no such debt remains, but the
# check stays as a ratchet.

set -euo pipefail

BASELINE_MISSING_DEVICE_CPP=0    # all capabilities have *_device.cpp + *_host.cpp.

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$repo_root"

cap_hdr_dir="$repo_root/components/blusys/include/blusys/framework/capabilities"
cap_src_dir="$repo_root/components/blusys/src/framework/capabilities"

missing=0
offenders=()

shopt -s nullglob
for hdr in "$cap_hdr_dir"/*.hpp; do
    name="$(basename "$hdr" .hpp)"
    case "$name" in
        capability|capabilities|event|event_table|list|inline|capability_base)
            continue;;  # shared infra, not a capability
        mqtt_host|telemetry)
            continue;;  # mqtt_host is host-only; telemetry is platform-agnostic (single .cpp)
    esac

    host_cpp="$cap_src_dir/${name}_host.cpp"
    device_cpp="$cap_src_dir/${name}_device.cpp"
    legacy_cpp="$cap_src_dir/${name}.cpp"

    if [ ! -f "$host_cpp" ]; then
        offenders+=("missing host: $host_cpp")
    fi
    if [ ! -f "$device_cpp" ]; then
        if [ -f "$legacy_cpp" ]; then
            missing=$((missing + 1))
            offenders+=("rename debt: $legacy_cpp → ${name}_device.cpp")
        else
            offenders+=("missing device: $device_cpp")
        fi
    fi
done

if [ "$missing" -gt "$BASELINE_MISSING_DEVICE_CPP" ] || \
   [ "${#offenders[@]}" -gt "$BASELINE_MISSING_DEVICE_CPP" ]; then
    echo "check-capability-shape: FAIL — mismatches=${#offenders[@]}, rename-debt=$missing, baseline max=$BASELINE_MISSING_DEVICE_CPP" >&2
    printf '  %s\n' "${offenders[@]}" >&2
    exit 1
fi

if [ "$missing" -lt "$BASELINE_MISSING_DEVICE_CPP" ]; then
    echo "check-capability-shape: note — rename-debt=$missing < baseline $BASELINE_MISSING_DEVICE_CPP." >&2
    echo "  Tighten BASELINE_MISSING_DEVICE_CPP to $missing." >&2
fi

echo "check-capability-shape: OK — rename-debt=$missing (baseline max $BASELINE_MISSING_DEVICE_CPP)"
exit 0
