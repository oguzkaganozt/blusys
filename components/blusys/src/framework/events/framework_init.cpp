// Placeholder translation unit. Keeps the framework_sources list stable
// (see components/blusys/CMakeLists.txt and
// cmake/blusys_host_bridge.cmake). The previous no-op `init()` and
// `run_once()` entry points were removed; instantiate
// `blusys::runtime` directly.

#include "blusys/framework/framework.hpp"
