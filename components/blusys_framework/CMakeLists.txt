set(blusys_framework_sources
    src/core/framework.cpp
)

if(BLUSYS_BUILD_UI)
    # UI sources are added here as the framework UI layer lands.
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
