#ifndef BLUSYS_VERSION_H
#define BLUSYS_VERSION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BLUSYS_VERSION_MAJOR 0
#define BLUSYS_VERSION_MINOR 1
#define BLUSYS_VERSION_PATCH 0
#define BLUSYS_VERSION_STRING "0.1.0-dev"

uint32_t blusys_version_packed(void);
const char *blusys_version_string(void);

#ifdef __cplusplus
}
#endif

#endif
