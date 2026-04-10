set(blusys_framework_sources
    src/core/controller.cpp
    src/core/feedback.cpp
    src/core/framework.cpp
    src/core/router.cpp
    src/core/runtime.cpp
    src/app/app_ctx.cpp
)

idf_build_get_property(build_components BUILD_COMPONENTS)

if(BLUSYS_BUILD_UI)
    if("lvgl__lvgl" IN_LIST build_components)
        list(APPEND blusys_framework_sources
            src/app/theme_presets.cpp
            src/app/action_widgets.cpp
            src/app/bindings.cpp
            src/app/overlay_manager.cpp
            src/app/page.cpp
            src/ui/input/encoder.cpp
            src/ui/primitives/col.cpp
            src/ui/primitives/divider.cpp
            src/ui/primitives/label.cpp
            src/ui/primitives/row.cpp
            src/ui/primitives/screen.cpp
            src/ui/theme.cpp
            src/ui/widgets/button/button.cpp
            src/ui/widgets/modal/modal.cpp
            src/ui/widgets/overlay/overlay.cpp
            src/ui/widgets/slider/slider.cpp
            src/ui/widgets/toggle/toggle.cpp
        )
    else()
        message(FATAL_ERROR "BLUSYS_BUILD_UI=ON requires the lvgl/lvgl managed component")
    endif()
endif()

idf_component_register(
    SRCS ${blusys_framework_sources}
    INCLUDE_DIRS include
    REQUIRES blusys blusys_services
)

target_compile_options(${COMPONENT_LIB} PRIVATE
    -std=c++20
    -fno-exceptions
    -fno-rtti
    -fno-threadsafe-statics
    -Os
    -ffunction-sections
    -fdata-sections
    -Wall
    -Wextra
)

if(BLUSYS_BUILD_UI)
    idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
    target_link_libraries(${COMPONENT_LIB} PUBLIC ${lvgl_lib})
    target_compile_definitions(${COMPONENT_LIB} PRIVATE BLUSYS_FRAMEWORK_HAS_UI=1)
endif()
