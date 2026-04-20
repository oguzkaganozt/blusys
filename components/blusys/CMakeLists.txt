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
set(blusys_sources)
set(BLUSYS_COMPONENT_SRC_DIR "${CMAKE_CURRENT_LIST_DIR}/src")

# Layered source manifests live in components/blusys/sources/ so the
# top-level component file stays readable without losing explicitness.
include("${CMAKE_CURRENT_LIST_DIR}/sources/hal.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/sources/drivers.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/sources/services.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/sources/framework.cmake")

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
