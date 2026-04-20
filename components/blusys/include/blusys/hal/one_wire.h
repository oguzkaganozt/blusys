/**
 * @file one_wire.h
 * @brief Dallas/Maxim 1-Wire bus: bus primitives + ROM commands + CRC-8 utility.
 *
 * The pin must support open-drain output (all standard GPIOs on ESP32/C3/S3
 * do). Connect a 4.7 kΩ pull-up resistor from the pin to 3.3 V — the internal
 * pull-up is too weak (~45 kΩ) for reliable 1-Wire communication. See
 * docs/hal/one_wire.md.
 */

#ifndef BLUSYS_ONE_WIRE_H
#define BLUSYS_ONE_WIRE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Opaque 1-Wire bus handle. */
typedef struct blusys_one_wire blusys_one_wire_t;

/** @brief 64-bit ROM code: `[0]` = family, `[1..6]` = 48-bit serial, `[7]` = CRC-8. */
typedef struct {
    uint8_t bytes[8];  /**< Raw ROM bytes (family + serial + CRC-8). */
} blusys_one_wire_rom_t;

/** @brief 1-Wire bus configuration. */
typedef struct {
    bool parasite_power;  /**< Devices draw power from the data line (no separate VDD). */
} blusys_one_wire_config_t;

/**
 * @brief Open a 1-Wire bus on the given pin.
 * @param pin     GPIO supporting open-drain output.
 * @param config  Bus configuration.
 * @param out_ow  Output handle.
 * @return `BLUSYS_OK`, `BLUSYS_ERR_INVALID_ARG` for invalid arguments.
 */
blusys_err_t blusys_one_wire_open(int pin,
                                   const blusys_one_wire_config_t *config,
                                   blusys_one_wire_t **out_ow);

/** @brief Release the 1-Wire bus and free its handle. */
blusys_err_t blusys_one_wire_close(blusys_one_wire_t *ow);

/**
 * @brief Send a reset pulse and wait for a presence pulse.
 * @return `BLUSYS_OK` if at least one device responded, `BLUSYS_ERR_NOT_FOUND` otherwise.
 */
blusys_err_t blusys_one_wire_reset(blusys_one_wire_t *ow);

/** @brief Write a single bit (LSB first within a byte). */
blusys_err_t blusys_one_wire_write_bit(blusys_one_wire_t *ow, bool bit);

/** @brief Read a single bit. */
blusys_err_t blusys_one_wire_read_bit(blusys_one_wire_t *ow, bool *out_bit);

/** @brief Write one byte, LSB first. */
blusys_err_t blusys_one_wire_write_byte(blusys_one_wire_t *ow, uint8_t byte);

/** @brief Read one byte, LSB first. */
blusys_err_t blusys_one_wire_read_byte(blusys_one_wire_t *ow, uint8_t *out_byte);

/** @brief Write a buffer of bytes. */
blusys_err_t blusys_one_wire_write(blusys_one_wire_t *ow, const uint8_t *data, size_t len);

/** @brief Read a buffer of bytes. */
blusys_err_t blusys_one_wire_read(blusys_one_wire_t *ow, uint8_t *data, size_t len);

/**
 * @brief READ ROM (0x33): read the 64-bit ROM code from the sole device on the bus.
 *
 * Only valid when exactly one device is connected.
 *
 * @return `BLUSYS_OK`, `BLUSYS_ERR_IO` if the CRC check fails.
 */
blusys_err_t blusys_one_wire_read_rom(blusys_one_wire_t *ow,
                                       blusys_one_wire_rom_t *out_rom);

/**
 * @brief SKIP ROM (0xCC): address all devices simultaneously.
 *
 * Issues a reset + SKIP ROM command; caller sends the function command next.
 */
blusys_err_t blusys_one_wire_skip_rom(blusys_one_wire_t *ow);

/**
 * @brief MATCH ROM (0x55): address one specific device by its 64-bit ROM code.
 *
 * Issues a reset + MATCH ROM + 8-byte address; caller sends the function command next.
 */
blusys_err_t blusys_one_wire_match_rom(blusys_one_wire_t *ow,
                                        const blusys_one_wire_rom_t *rom);

/**
 * @brief SEARCH ROM (0xF0): enumerate devices one at a time.
 *
 * Call repeatedly until `*out_last_device` is true or `BLUSYS_ERR_NOT_FOUND`
 * is returned. Call ::blusys_one_wire_search_reset before starting a new
 * enumeration pass.
 *
 * @warning Must be called from a single task — search state is not
 *          concurrency-safe.
 */
blusys_err_t blusys_one_wire_search(blusys_one_wire_t *ow,
                                     blusys_one_wire_rom_t *out_rom,
                                     bool *out_last_device);

/** @brief Reset SEARCH ROM state so the next ::blusys_one_wire_search starts over. */
blusys_err_t blusys_one_wire_search_reset(blusys_one_wire_t *ow);

/**
 * @brief Compute Dallas/Maxim CRC-8 (polynomial 0x31, reflected as 0x8C) over @p len bytes.
 *
 * To verify a ROM code: `blusys_one_wire_crc8(rom.bytes, 7)` should equal
 * `rom.bytes[7]`.
 *
 * @param data  Byte buffer.
 * @param len   Number of bytes.
 * @return CRC-8 value.
 */
uint8_t blusys_one_wire_crc8(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* BLUSYS_ONE_WIRE_H */
