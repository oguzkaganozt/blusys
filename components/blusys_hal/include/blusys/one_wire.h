#ifndef BLUSYS_ONE_WIRE_H
#define BLUSYS_ONE_WIRE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "blusys/error.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Opaque handle */
typedef struct blusys_one_wire blusys_one_wire_t;

/* 64-bit ROM code: [0]=family code, [1..6]=48-bit serial, [7]=CRC-8 */
typedef struct {
    uint8_t bytes[8];
} blusys_one_wire_rom_t;

typedef struct {
    bool parasite_power; /* devices draw power from the data line (no separate VDD) */
} blusys_one_wire_config_t;

/*
 * Lifecycle
 *
 * The pin must support open-drain output (all standard GPIOs on ESP32/C3/S3 do).
 * Connect a 4.7 kΩ pull-up resistor from the pin to 3.3 V — the internal pull-up
 * is too weak (~45 kΩ) for reliable 1-Wire communication.
 */
blusys_err_t blusys_one_wire_open(int pin,
                                   const blusys_one_wire_config_t *config,
                                   blusys_one_wire_t **out_ow);
blusys_err_t blusys_one_wire_close(blusys_one_wire_t *ow);

/*
 * Bus primitives
 */

/* Send a reset pulse and wait for a presence pulse.
   Returns BLUSYS_OK if at least one device responded, BLUSYS_ERR_NOT_FOUND otherwise. */
blusys_err_t blusys_one_wire_reset(blusys_one_wire_t *ow);

/* Write a single bit (LSB first within a byte). */
blusys_err_t blusys_one_wire_write_bit(blusys_one_wire_t *ow, bool bit);

/* Read a single bit. */
blusys_err_t blusys_one_wire_read_bit(blusys_one_wire_t *ow, bool *out_bit);

/* Write one byte, LSB first. */
blusys_err_t blusys_one_wire_write_byte(blusys_one_wire_t *ow, uint8_t byte);

/* Read one byte, LSB first. */
blusys_err_t blusys_one_wire_read_byte(blusys_one_wire_t *ow, uint8_t *out_byte);

/* Write a buffer of bytes. */
blusys_err_t blusys_one_wire_write(blusys_one_wire_t *ow, const uint8_t *data, size_t len);

/* Read a buffer of bytes. */
blusys_err_t blusys_one_wire_read(blusys_one_wire_t *ow, uint8_t *data, size_t len);

/*
 * ROM commands
 */

/* READ ROM (0x33): read the 64-bit ROM code from the sole device on the bus.
   Returns BLUSYS_ERR_IO if the CRC check fails.
   Only valid when exactly one device is connected. */
blusys_err_t blusys_one_wire_read_rom(blusys_one_wire_t *ow,
                                       blusys_one_wire_rom_t *out_rom);

/* SKIP ROM (0xCC): address all devices simultaneously.
   Issues a reset + SKIP ROM command; caller sends the function command next. */
blusys_err_t blusys_one_wire_skip_rom(blusys_one_wire_t *ow);

/* MATCH ROM (0x55): address one specific device by its 64-bit ROM code.
   Issues a reset + MATCH ROM + 8-byte address; caller sends the function command next. */
blusys_err_t blusys_one_wire_match_rom(blusys_one_wire_t *ow,
                                        const blusys_one_wire_rom_t *rom);

/* SEARCH ROM (0xF0): enumerate devices one at a time.
   Call repeatedly until *out_last_device is true or BLUSYS_ERR_NOT_FOUND is returned.
   Call blusys_one_wire_search_reset() before starting a new enumeration pass.
   Must be called from a single task — search state is not concurrency-safe. */
blusys_err_t blusys_one_wire_search(blusys_one_wire_t *ow,
                                     blusys_one_wire_rom_t *out_rom,
                                     bool *out_last_device);

/* Reset SEARCH ROM state so the next call to blusys_one_wire_search() starts over. */
blusys_err_t blusys_one_wire_search_reset(blusys_one_wire_t *ow);

/*
 * Utility
 */

/* Compute Dallas/Maxim CRC-8 (polynomial 0x31, reflected as 0x8C) over len bytes.
   To verify a ROM code: blusys_one_wire_crc8(rom.bytes, 7) should equal rom.bytes[7]. */
uint8_t blusys_one_wire_crc8(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* BLUSYS_ONE_WIRE_H */
