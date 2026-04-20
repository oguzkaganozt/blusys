/**
 * @file version.h
 * @brief Compile-time and runtime Blusys version identifiers.
 *
 * The `BLUSYS_VERSION_*` macros hold the version of the Blusys component this
 * firmware was built against; the functions return the same values at runtime
 * so shared libraries or loaded modules can query the host's version.
 */

#ifndef BLUSYS_VERSION_H
#define BLUSYS_VERSION_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define BLUSYS_VERSION_MAJOR 7           /**< Major component version. */
#define BLUSYS_VERSION_MINOR 0           /**< Minor component version. */
#define BLUSYS_VERSION_PATCH 0           /**< Patch component version. */
#define BLUSYS_VERSION_STRING "7.0.0"    /**< Human-readable version, matches the MAJOR.MINOR.PATCH triple. */

/**
 * @brief Blusys version packed as `MAJOR << 16 | MINOR << 8 | PATCH`.
 *
 * Convenient for numeric comparisons: `blusys_version_packed() >= (7u << 16)`.
 */
uint32_t blusys_version_packed(void);

/** @brief Blusys version as a null-terminated `"MAJOR.MINOR.PATCH"` string. */
const char *blusys_version_string(void);

#ifdef __cplusplus
}
#endif

#endif
