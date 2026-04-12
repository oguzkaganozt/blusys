# Optional Espressif managed components register under either the short name
# (e.g. mdns) or the mangled form (espressif__mdns) in BUILD_COMPONENTS.
#
# Caller must run first:
#   idf_build_get_property(build_components BUILD_COMPONENTS)
#
# Sets OUT_VAR to the resolved component name, or empty if neither is present.
macro(blusys_optional_managed_component OUT_VAR SHORT_NAME ESPRESSIF_MANGLED)
    set(${OUT_VAR} "")
    if("${SHORT_NAME}" IN_LIST build_components)
        set(${OUT_VAR} "${SHORT_NAME}")
    elseif("${ESPRESSIF_MANGLED}" IN_LIST build_components)
        set(${OUT_VAR} "${ESPRESSIF_MANGLED}")
    endif()
endmacro()
