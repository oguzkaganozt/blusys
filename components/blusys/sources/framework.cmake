list(APPEND blusys_sources
    ${BLUSYS_COMPONENT_SRC_DIR}/observe/error_domain.c
    ${BLUSYS_COMPONENT_SRC_DIR}/observe/log.c
    ${BLUSYS_COMPONENT_SRC_DIR}/observe/counter.c
    ${BLUSYS_COMPONENT_SRC_DIR}/observe/snapshot.c
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/events/router.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/events/event_queue.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/events/framework_init.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/feedback/feedback.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/feedback/presets.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/feedback/internal/logging_sink.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/capabilities/capability_event_map.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/flows/boot.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/flows/connectivity_flow.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/flows/diagnostics_flow.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/flows/error.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/flows/loading.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/flows/ota_flow.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/flows/provisioning_flow.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/flows/settings.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/flows/status.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/platform/input_bridge.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/platform/touch_bridge.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/platform/button_array_bridge.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/platform/app_device_platform.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/platform/app_headless_platform_esp.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/app/ctx.cpp
    # Host stub implementations (*_host.cpp) are excluded here — the
    # host CMake bridge (cmake/blusys_host_bridge.cmake) picks them up
    # for PC builds. mqtt_host keeps its own class on host and is
    # excluded from the device build entirely.
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/capabilities/ble_hid_device_device.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/capabilities/build_info_device.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/capabilities/example_sensor_device.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/capabilities/bluetooth_device.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/capabilities/connectivity_device.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/capabilities/diagnostics_device.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/capabilities/lan_control_device.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/capabilities/mqtt_device.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/capabilities/ota_device.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/capabilities/persistence_device.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/capabilities/provisioning_device.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/capabilities/storage_device.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/capabilities/telemetry.cpp
    ${BLUSYS_COMPONENT_SRC_DIR}/framework/capabilities/usb_device.cpp
)
