#include <stdio.h>

#include <stdint.h>

#include "sdkconfig.h"

#include "blusys/blusys.h"

#define BLUSYS_I2C_SCAN_PORT CONFIG_BLUSYS_I2C_SCAN_PORT
#define BLUSYS_I2C_SCAN_SDA_PIN CONFIG_BLUSYS_I2C_SCAN_SDA_PIN
#define BLUSYS_I2C_SCAN_SCL_PIN CONFIG_BLUSYS_I2C_SCAN_SCL_PIN

#define BLUSYS_I2C_SCAN_FREQ_HZ 100000u
#define BLUSYS_I2C_SCAN_TIMEOUT_MS 20

void app_main(void)
{
    blusys_i2c_master_t *i2c = NULL;
    blusys_err_t err;
    unsigned int found_count = 0u;

    printf("Blusys i2c scan\n");
    printf("target: %s\n", blusys_target_name());
    printf("port: %d\n", BLUSYS_I2C_SCAN_PORT);
    printf("sda_pin: %d\n", BLUSYS_I2C_SCAN_SDA_PIN);
    printf("scl_pin: %d\n", BLUSYS_I2C_SCAN_SCL_PIN);
    printf("connect pull-ups and set board-safe I2C pins in menuconfig if needed\n");

    err = blusys_i2c_master_open(BLUSYS_I2C_SCAN_PORT,
                                 BLUSYS_I2C_SCAN_SDA_PIN,
                                 BLUSYS_I2C_SCAN_SCL_PIN,
                                 BLUSYS_I2C_SCAN_FREQ_HZ,
                                 &i2c);
    if (err != BLUSYS_OK) {
        printf("open error: %s\n", blusys_err_string(err));
        return;
    }

    for (uint16_t address = 0x08u; address < 0x78u; ++address) {
        err = blusys_i2c_master_probe(i2c, address, BLUSYS_I2C_SCAN_TIMEOUT_MS);
        if (err == BLUSYS_OK) {
            printf("found: 0x%02x\n", (unsigned int) address);
            ++found_count;
            continue;
        }

        if (err == BLUSYS_ERR_TIMEOUT) {
            printf("probe timeout at 0x%02x\n", (unsigned int) address);
            break;
        }
    }

    printf("devices_found: %u\n", found_count);

    err = blusys_i2c_master_close(i2c);
    if (err != BLUSYS_OK) {
        printf("close error: %s\n", blusys_err_string(err));
    }
}
