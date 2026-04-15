#include "sdkconfig.h"

#if CONFIG_REF_HAL_REFERENCE_SCENARIO_I2C_SCAN

#include <stdio.h>

#include <stdint.h>


#include "blusys/blusys.h"

#define BLUSYS_I2C_SCAN_PORT CONFIG_BLUSYS_I2C_SCAN_PORT
#define BLUSYS_I2C_SCAN_SDA_PIN CONFIG_BLUSYS_I2C_SCAN_SDA_PIN
#define BLUSYS_I2C_SCAN_SCL_PIN CONFIG_BLUSYS_I2C_SCAN_SCL_PIN

#define BLUSYS_I2C_SCAN_FREQ_HZ 100000u
#define BLUSYS_I2C_SCAN_TIMEOUT_MS 20

static bool blusys_i2c_scan_on_device(uint16_t device_address, void *user_ctx)
{
    unsigned int *found_count = user_ctx;

    printf("found: 0x%02x\n", (unsigned int) device_address);
    *found_count += 1u;

    return true;
}

void run_i2c_scan(void)
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

    err = blusys_i2c_master_scan(i2c,
                                 0x08u,
                                 0x77u,
                                 BLUSYS_I2C_SCAN_TIMEOUT_MS,
                                 blusys_i2c_scan_on_device,
                                 &found_count,
                                 NULL);
    if (err != BLUSYS_OK) {
        printf("scan error: %s\n", blusys_err_string(err));
    }

    printf("devices_found: %u\n", found_count);

    err = blusys_i2c_master_close(i2c);
    if (err != BLUSYS_OK) {
        printf("close error: %s\n", blusys_err_string(err));
    }
}


#else

void run_i2c_scan(void) {}

#endif /* CONFIG_REF_HAL_REFERENCE_SCENARIO_I2C_SCAN */
