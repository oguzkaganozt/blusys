cmake_minimum_required(VERSION 3.16)

get_filename_component(BLUSYS_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(EXTRA_COMPONENT_DIRS "${BLUSYS_REPO_ROOT}/components")
set(BLUSYS_BUILD_UI OFF CACHE BOOL "Build blusys/framework/ui" FORCE)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(blusys_lan_control_basic)
