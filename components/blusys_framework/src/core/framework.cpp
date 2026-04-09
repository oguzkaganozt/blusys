#include "blusys/framework/framework.hpp"

#include "blusys/log.h"

namespace blusys::framework {
namespace {

constexpr const char *kTag = "blusys_framework";

}  // namespace

void init()
{
    BLUSYS_LOGD(kTag, "framework init placeholder");
}

void run_once()
{
}

}  // namespace blusys::framework
