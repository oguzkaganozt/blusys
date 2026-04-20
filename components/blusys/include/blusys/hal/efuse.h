/**
 * @file efuse.h
 * @brief Read-only access to factory-programmed chip identity and security state.
 *
 * Exposes a small common subset of eFuse fields (base MAC, chip/package
 * revision, secure version, security flags). Blusys does not expose eFuse
 * programming APIs. See docs/hal/efuse.md.
 */

#ifndef BLUSYS_EFUSE_H
#define BLUSYS_EFUSE_H

#include <stdbool.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Length in bytes of the factory-programmed base MAC address. */
#define BLUSYS_EFUSE_MAC_LEN 6

/**
 * @brief Read the factory-programmed base MAC address.
 * @param out_mac  Output buffer of at least ::BLUSYS_EFUSE_MAC_LEN bytes.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p out_mac is NULL, or a
 *         translated ESP-IDF error if the MAC cannot be read.
 */
blusys_err_t blusys_efuse_base_mac(uint8_t out_mac[BLUSYS_EFUSE_MAC_LEN]);

/**
 * @brief Chip revision encoded in ESP-IDF's `MXX` format.
 * @param out_revision  Output pointer for the revision value.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p out_revision is NULL.
 */
blusys_err_t blusys_efuse_chip_revision(uint16_t *out_revision);

/**
 * @brief Package version reported by the chip's eFuse metadata.
 * @param out_version  Output pointer for the package version value.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p out_version is NULL.
 */
blusys_err_t blusys_efuse_package_version(uint32_t *out_version);

/**
 * @brief Anti-rollback secure version (as written by the bootloader).
 * @param out_version  Output pointer for the secure version value.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p out_version is NULL.
 */
blusys_err_t blusys_efuse_secure_version(uint32_t *out_version);

/**
 * @brief Whether flash encryption is enabled in hardware.
 * @param out_enabled  Output pointer for the flag.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p out_enabled is NULL.
 */
blusys_err_t blusys_efuse_flash_encryption_enabled(bool *out_enabled);

/**
 * @brief Whether secure boot is enabled in hardware.
 * @param out_enabled  Output pointer for the flag.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` if @p out_enabled is NULL.
 */
blusys_err_t blusys_efuse_secure_boot_enabled(bool *out_enabled);

#ifdef __cplusplus
}
#endif

#endif
