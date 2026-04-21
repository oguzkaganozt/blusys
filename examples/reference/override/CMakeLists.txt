cmake_minimum_required(VERSION 3.16)

set(BLUSYS_BUILD_UI OFF CACHE BOOL "Build blusys/framework/ui" FORCE)

get_filename_component(BLUSYS_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../../.." ABSOLUTE)
set(EXTRA_COMPONENT_DIRS "${BLUSYS_REPO_ROOT}/components")

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(override_reference)
