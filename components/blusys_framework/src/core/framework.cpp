// Umbrella symbols for blusys::framework (see framework.hpp). The product path
// is driven by blusys::app and core/runtime.hpp; these entry points stay minimal
// so validation and widget demos can include framework.hpp without side effects.

#include "blusys/framework/framework.hpp"

#include "blusys/log.h"

namespace blusys::framework {
namespace {

constexpr const char *kTag = "blusys_framework";

}  // namespace

void init()
{
    BLUSYS_LOGD(kTag, "framework init (no-op)");
}

void run_once()
{
}

}  // namespace blusys::framework
