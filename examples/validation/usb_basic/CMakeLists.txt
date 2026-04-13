cmake_minimum_required(VERSION 3.16)

get_filename_component(BLUSYS_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(EXTRA_COMPONENT_DIRS "${BLUSYS_REPO_ROOT}/components")

set(SUPPORTED_TARGETS esp32s3)
if(DEFINED IDF_TARGET AND NOT "${IDF_TARGET}" STREQUAL "esp32s3")
    message(FATAL_ERROR "usb_basic requires USB OTG (ESP32-S3 only). Current target: ${IDF_TARGET}")
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(blusys_usb_basic)
