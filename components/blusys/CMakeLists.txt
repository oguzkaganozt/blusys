# components/blusys/CMakeLists.txt
#
# Single merged platform component — v0 architecture.
#
# Layer dependency order (one-way):
#   hal      → (nothing)
#   drivers  → hal
#   services → hal + drivers
#   framework → (pure; only framework/platform/ may bridge to layers below)

cmake_minimum_required(VERSION 3.16)

# Resolve the build component list early — needed to check for optional
# managed components (LVGL, tinyusb, etc.) before idf_component_register.
idf_build_get_property(build_components BUILD_COMPONENTS)

execute_process(
    COMMAND git -C "${CMAKE_CURRENT_LIST_DIR}/../.." rev-parse --short=12 HEAD
    OUTPUT_VARIABLE BLUSYS_GIT_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT BLUSYS_GIT_COMMIT)
    set(BLUSYS_GIT_COMMIT "unknown")
endif()

execute_process(
    COMMAND hostname
    OUTPUT_VARIABLE BLUSYS_BUILD_HOST
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(NOT BLUSYS_BUILD_HOST)
    set(BLUSYS_BUILD_HOST "unknown")
endif()

# ---------------------------------------------------------------------------
# Sources
# ---------------------------------------------------------------------------
set(blusys_sources
    # --- HAL layer ---
    src/hal/adc.c
    src/hal/dac.c
    src/hal/efuse.c
    src/hal/error.c
    src/hal/gpio.c
    src/hal/gpio_expander.c
    src/hal/i2c.c
    src/hal/i2c_slave.c
    src/hal/i2s.c
    src/hal/i2s_rx.c
    src/hal/mcpwm.c
    src/hal/nvs.c
    src/hal/one_wire.c
    src/hal/pcnt.c
    src/hal/pwm.c
    src/hal/rmt.c
    src/hal/rmt_rx.c
    src/hal/sd_spi.c
    src/hal/sdm.c
    src/hal/sdmmc.c
    src/hal/sleep.c
    src/hal/spi.c
    src/hal/spi_slave.c
    src/hal/system.c
    src/hal/target.c
    src/hal/temp_sensor.c
    src/hal/timer.c
    src/hal/touch.c
    src/hal/twai.c
    src/hal/uart.c
    src/hal/usb_device.c
    src/hal/usb_host.c
    src/hal/version.c
    src/hal/wdt.c
    src/hal/internal/bt_stack.c
    src/hal/internal/lock_freertos.c
    # --- Drivers layer ---
    src/drivers/button.c
    src/drivers/buzzer.c
    src/drivers/dht.c
    src/drivers/encoder.c
    src/drivers/lcd.c
    src/drivers/lcd_panel_ili.c
    src/drivers/led_strip.c
    src/drivers/seven_seg.c
    src/drivers/display.c
    # --- Services layer ---
    src/services/connectivity/ble_gatt.c
    src/services/connectivity/ble_hid_device.c
    src/services/connectivity/bluetooth.c
    src/services/connectivity/espnow.c
    src/services/protocol/http_client.c
    src/services/protocol/http_server.c
    src/services/protocol/local_ctrl.c
    src/services/protocol/mdns.c
    src/services/connectivity/usb_hid.c
    src/services/connectivity/wifi.c
    src/services/connectivity/wifi_mesh.c
    src/services/connectivity/wifi_prov.c
    src/services/protocol/mqtt.c
    src/services/protocol/sntp.c
    src/services/protocol/ws_client.c
    src/services/storage/fatfs.c
    src/services/storage/fs.c
    src/services/system/console.c
    src/services/system/ota.c
    src/services/system/power_mgmt.c
    # --- Framework internal service helpers ---
    src/services/internal/net_bootstrap.c
    src/services/internal/storage_helpers.c
    # --- Framework layer ---
    src/observe/error_domain.c
    src/observe/log.c
    src/observe/counter.c
    src/observe/snapshot.c
    src/framework/events/router.cpp
    src/framework/events/event_queue.cpp
    src/framework/events/framework_init.cpp
    src/framework/feedback/feedback.cpp
    src/framework/feedback/presets.cpp
    src/framework/feedback/internal/logging_sink.cpp
    src/framework/capabilities/capability_event_map.cpp
    src/framework/flows/boot.cpp
    src/framework/flows/connectivity_flow.cpp
    src/framework/flows/diagnostics_flow.cpp
    src/framework/flows/error.cpp
    src/framework/flows/loading.cpp
    src/framework/flows/ota_flow.cpp
    src/framework/flows/provisioning_flow.cpp
    src/framework/flows/settings.cpp
    src/framework/flows/status.cpp
    # --- Framework platform layer ---
    src/framework/platform/input_bridge.cpp
    src/framework/platform/touch_bridge.cpp
    src/framework/platform/button_array_bridge.cpp
    src/framework/platform/app_device_platform.cpp
    src/framework/platform/app_headless_platform_esp.cpp
    src/framework/app/ctx.cpp
    # --- Framework capabilities (ESP-IDF / device implementations) ---
    # Host stub implementations (*_host.cpp) are excluded here — the
    # host CMake bridge (cmake/blusys_host_bridge.cmake) picks them up
    # for PC builds. mqtt_host keeps its own class on host and is
    # excluded from the device build entirely.
    src/framework/capabilities/ble_hid_device_device.cpp
    src/framework/capabilities/build_info_device.cpp
    src/framework/capabilities/example_sensor_device.cpp
    src/framework/capabilities/bluetooth_device.cpp
    src/framework/capabilities/connectivity_device.cpp
    src/framework/capabilities/diagnostics_device.cpp
    src/framework/capabilities/lan_control_device.cpp
    src/framework/capabilities/mqtt_device.cpp
    src/framework/capabilities/ota_device.cpp
    src/framework/capabilities/persistence_device.cpp
    src/framework/capabilities/provisioning_device.cpp
    src/framework/capabilities/storage_device.cpp
    src/framework/capabilities/telemetry.cpp
    src/framework/capabilities/usb_device.cpp
)

# Target capability table (HAL). Single TU; chooses the right struct via
# CONFIG_IDF_TARGET_* at compile time.
if(NOT IDF_TARGET MATCHES "^(esp32|esp32c3|esp32s3)$")
    message(FATAL_ERROR "blusys currently supports only esp32, esp32c3, and esp32s3")
endif()
list(APPEND blusys_sources src/hal/target_caps.c)

# ---------------------------------------------------------------------------
# ESP-IDF component registration
# ---------------------------------------------------------------------------
idf_component_register(
    SRCS ${blusys_sources}
    INCLUDE_DIRS include
    REQUIRES
        # HAL dependencies
        freertos efuse esp_adc esp_driver_dac esp_driver_gpio
        esp_driver_gptimer esp_driver_i2c esp_driver_i2s esp_driver_ledc
        esp_driver_mcpwm esp_driver_pcnt esp_driver_rmt esp_driver_sdm
        esp_driver_sdmmc esp_driver_sdspi esp_driver_spi
        esp_driver_touch_sens esp_driver_tsens esp_driver_twai
        esp_driver_uart esp_hw_support esp_system esp_timer
        bootloader_support fatfs nvs_flash sdmmc ulp usb esp_lcd
        # Services dependencies
        console esp_wifi esp_netif esp_event
        esp_http_client esp_http_server mqtt esp_https_ota app_update
        bt spiffs wear_levelling esp_pm tcp_transport
        wifi_provisioning protocomm
        esp_partition mbedtls
        # Framework dependencies
        spi_flash
)

# ---------------------------------------------------------------------------
# C++ compile options (framework layer)
# ---------------------------------------------------------------------------
if(blusys_sources)
    target_compile_options(${COMPONENT_LIB} PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:
            -std=c++20
            -fno-exceptions
            -fno-rtti
            -fno-threadsafe-statics
            -Os
            -ffunction-sections
            -fdata-sections
            -Wall
            -Wextra
        >
    )
    target_compile_definitions(${COMPONENT_LIB} PUBLIC
        BLUSYS_BUILD_HOST="${BLUSYS_BUILD_HOST}"
        BLUSYS_BUILD_COMMIT="${BLUSYS_GIT_COMMIT}"
        BLUSYS_DEVICE_BUILD=1
    )
endif()

# ---------------------------------------------------------------------------
# Optional managed components
# ---------------------------------------------------------------------------
include("${CMAKE_CURRENT_LIST_DIR}/../../cmake/blusys_optional_component.cmake")

# tinyusb (USB device stack)
blusys_optional_managed_component(blusys_tinyusb_component "esp_tinyusb" "espressif__esp_tinyusb")
if(NOT blusys_tinyusb_component STREQUAL "")
    idf_component_get_property(tinyusb_lib ${blusys_tinyusb_component} COMPONENT_LIB)
    target_link_libraries(${COMPONENT_LIB} PUBLIC ${tinyusb_lib})
    target_compile_definitions(${COMPONENT_LIB} PRIVATE BLUSYS_HAS_TINYUSB=1)
endif()

# espressif/esp_lcd_qemu_rgb (QEMU virtual display)
blusys_optional_managed_component(blusys_esp_lcd_qemu_rgb "esp_lcd_qemu_rgb" "espressif__esp_lcd_qemu_rgb")
if(NOT blusys_esp_lcd_qemu_rgb STREQUAL "")
    idf_component_get_property(esp_lcd_qemu_rgb_lib ${blusys_esp_lcd_qemu_rgb} COMPONENT_LIB)
    target_link_libraries(${COMPONENT_LIB} PUBLIC ${esp_lcd_qemu_rgb_lib})
    target_compile_definitions(${COMPONENT_LIB} PRIVATE BLUSYS_HAS_ESP_LCD_QEMU_RGB=1)
endif()

# espressif/mdns
blusys_optional_managed_component(blusys_mdns_component "mdns" "espressif__mdns")
if(NOT blusys_mdns_component STREQUAL "")
    idf_component_get_property(mdns_lib ${blusys_mdns_component} COMPONENT_LIB)
    target_link_libraries(${COMPONENT_LIB} PUBLIC ${mdns_lib})
    target_compile_definitions(${COMPONENT_LIB} PRIVATE BLUSYS_HAS_MDNS=1)
endif()

# espressif/usb_host_hid
blusys_optional_managed_component(blusys_usb_host_hid_component "usb_host_hid" "espressif__usb_host_hid")
if(NOT blusys_usb_host_hid_component STREQUAL "")
    idf_component_get_property(usb_host_hid_lib ${blusys_usb_host_hid_component} COMPONENT_LIB)
    target_link_libraries(${COMPONENT_LIB} PUBLIC ${usb_host_hid_lib})
    target_compile_definitions(${COMPONENT_LIB} PRIVATE BLUSYS_HAS_USB_HOST_HID=1)
endif()

# lvgl/lvgl (framework UI)
# UI sources include lvgl.h directly — only compile them when LVGL is present.
# BLUSYS_FRAMEWORK_HAS_UI is PUBLIC so product code and framework headers can
# gate on it (e.g. framework.hpp, entry.hpp).
if("lvgl__lvgl" IN_LIST build_components)
    idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
    target_sources(${COMPONENT_LIB} PRIVATE
        src/framework/ui/controller/navigation_controller.cpp
        src/framework/ui/composition/overlay_manager.cpp
        src/framework/ui/composition/page.cpp
        src/framework/ui/composition/screen_registry.cpp
        src/framework/ui/composition/screen_router.cpp
        src/framework/ui/composition/shell.cpp
        src/framework/ui/binding/bindings.cpp
        src/framework/ui/binding/action_widgets.cpp
        src/framework/ui/style/presets.cpp
        src/framework/ui/style/theme.cpp
        src/framework/ui/style/transition.cpp
        src/framework/ui/style/interaction_effects.cpp
        src/framework/ui/style/icon_set_minimal.cpp
        src/framework/ui/input/encoder.cpp
        src/framework/ui/input/focus_scope.cpp
        src/framework/ui/primitives/col.cpp
        src/framework/ui/primitives/divider.cpp
        src/framework/ui/primitives/icon_label.cpp
        src/framework/ui/primitives/key_value.cpp
        src/framework/ui/primitives/label.cpp
        src/framework/ui/primitives/row.cpp
        src/framework/ui/primitives/screen.cpp
        src/framework/ui/primitives/status_badge.cpp
        src/framework/ui/widgets/button.cpp
        src/framework/ui/widgets/card.cpp
        src/framework/ui/widgets/chart.cpp
        src/framework/ui/widgets/data_table.cpp
        src/framework/ui/widgets/dropdown.cpp
        src/framework/ui/widgets/gauge.cpp
        src/framework/ui/widgets/input_field.cpp
        src/framework/ui/widgets/knob.cpp
        src/framework/ui/widgets/level_bar.cpp
        src/framework/ui/widgets/list.cpp
        src/framework/ui/widgets/modal.cpp
        src/framework/ui/widgets/overlay.cpp
        src/framework/ui/widgets/progress.cpp
        src/framework/ui/widgets/slider.cpp
        src/framework/ui/widgets/tabs.cpp
        src/framework/ui/widgets/toggle.cpp
        src/framework/ui/widgets/vu_strip.cpp
        src/framework/ui/screens/about.cpp
        src/framework/ui/screens/diagnostics.cpp
        src/framework/ui/screens/status.cpp
    )
    target_link_libraries(${COMPONENT_LIB} PUBLIC ${lvgl_lib})
    target_compile_definitions(${COMPONENT_LIB} PUBLIC BLUSYS_FRAMEWORK_HAS_UI=1)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE BLUSYS_HAS_LVGL=1)
    target_compile_definitions(${COMPONENT_LIB} PUBLIC
        BLUSYS_BUILD_FEATURE_FLAGS="platform=device,ui=1")
else()
    target_compile_definitions(${COMPONENT_LIB} PUBLIC
        BLUSYS_BUILD_FEATURE_FLAGS="platform=device,ui=0")
endif()

# ---------------------------------------------------------------------------
# ULP (HAL)
# ---------------------------------------------------------------------------
if(CONFIG_ULP_COPROC_ENABLED AND CONFIG_ULP_COPROC_TYPE_FSM AND
   (IDF_TARGET STREQUAL "esp32" OR IDF_TARGET STREQUAL "esp32s3"))
    set(blusys_ulp_app_name blusys_ulp_gpio_watch)
    set(blusys_ulp_sources
        "src/hal/ulp/gpio_watch.S"
        "src/hal/ulp/wake_up.S"
    )
    set(blusys_ulp_dep_sources "src/hal/ulp/ulp.c")
    ulp_embed_binary(${blusys_ulp_app_name}
                     "${blusys_ulp_sources}"
                     "${blusys_ulp_dep_sources}"
                     PREFIX "blusys_ulp_gpio_watch_")
endif()
