# Optional host (SDL) MQTT sources — libmosquitto. Include after BLUSYS_PATH is set.
#
#   list(APPEND BLUSYS_FRAMEWORK_HOST_SOURCES ${BLUSYS_FRAMEWORK_HOST_MQTT_SOURCES})
#
set(BLUSYS_FRAMEWORK_HOST_MQTT_SOURCES
    "${BLUSYS_PATH}/components/blusys/src/framework/capabilities/mqtt_host.cpp"
)
