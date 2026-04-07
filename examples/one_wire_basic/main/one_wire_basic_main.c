#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "blusys/blusys.h"

#define ONE_WIRE_PIN CONFIG_BLUSYS_ONE_WIRE_BASIC_PIN

static void print_rom(const blusys_one_wire_rom_t *rom)
{
    printf("  family=0x%02X  serial=%02X:%02X:%02X:%02X:%02X:%02X  crc=0x%02X\n",
           rom->bytes[0],
           rom->bytes[1], rom->bytes[2], rom->bytes[3],
           rom->bytes[4], rom->bytes[5], rom->bytes[6],
           rom->bytes[7]);
}

void app_main(void)
{
    blusys_one_wire_t *ow = NULL;
    blusys_one_wire_rom_t rom;
    blusys_err_t err;
    bool last;
    int count;

    printf("Blusys one_wire basic\n");
    printf("target: %s\n", blusys_target_name());
    printf("pin: %d  (connect 4.7 kΩ pull-up to 3.3 V)\n\n", ONE_WIRE_PIN);

    if (!blusys_target_supports(BLUSYS_FEATURE_ONE_WIRE)) {
        printf("1-Wire not supported on this target\n");
        return;
    }

    err = blusys_one_wire_open(ONE_WIRE_PIN,
                               &(blusys_one_wire_config_t){ .parasite_power = false },
                               &ow);
    if (err != BLUSYS_OK) {
        printf("open failed: %s\n", blusys_err_string(err));
        return;
    }

    /* ── Single device: READ ROM ──────────────────────────────────── */
    printf("-- READ ROM (single device) --\n");
    err = blusys_one_wire_read_rom(ow, &rom);
    if (err == BLUSYS_OK) {
        print_rom(&rom);
    } else {
        printf("  read_rom: %s\n", blusys_err_string(err));
    }

    /* ── Multiple devices: SEARCH ROM ────────────────────────────── */
    printf("\n-- SEARCH ROM (enumerate all) --\n");
    blusys_one_wire_search_reset(ow);
    count = 0;
    last  = false;
    while (!last) {
        err = blusys_one_wire_search(ow, &rom, &last);
        if (err != BLUSYS_OK) {
            printf("  search: %s\n", blusys_err_string(err));
            break;
        }
        printf("[%d]", ++count);
        print_rom(&rom);
    }
    printf("found %d device(s)\n", count);

    blusys_one_wire_close(ow);
}
