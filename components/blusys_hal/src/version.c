#include "blusys/version.h"

#include "blusys/internal/blusys_internal.h"

uint32_t blusys_version_packed(void)
{
    return BLUSYS_VERSION_PACK(
        BLUSYS_VERSION_MAJOR,
        BLUSYS_VERSION_MINOR,
        BLUSYS_VERSION_PATCH
    );
}

const char *blusys_version_string(void)
{
    return BLUSYS_VERSION_STRING;
}
