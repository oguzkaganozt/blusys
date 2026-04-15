#if CONFIG_PERIPHERAL_LAB_SCENARIO_I2C_SLAVE
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "blusys/blusys.h"

#define BLUSYS_I2C_SLAVE_BASIC_SDA_PIN  CONFIG_BLUSYS_I2C_SLAVE_BASIC_SDA_PIN
#define BLUSYS_I2C_SLAVE_BASIC_SCL_PIN  CONFIG_BLUSYS_I2C_SLAVE_BASIC_SCL_PIN
#define BLUSYS_I2C_SLAVE_BASIC_ADDR     CONFIG_BLUSYS_I2C_SLAVE_BASIC_ADDR

#define BLUSYS_I2C_SLAVE_BASIC_TX_BUF   128u
#define BLUSYS_I2C_SLAVE_BASIC_RX_SIZE  32u
#define BLUSYS_I2C_SLAVE_BASIC_ROUNDS   5u
#define BLUSYS_I2C_SLAVE_BASIC_TIMEOUT_MS 10000

void run_periph_i2c_slave(void)
{
    blusys_i2c_slave_t *slave = NULL;
    blusys_err_t err;
    static uint8_t rx_buf[BLUSYS_I2C_SLAVE_BASIC_RX_SIZE];
    static uint8_t tx_buf[BLUSYS_I2C_SLAVE_BASIC_TX_BUF];
    size_t received;
    unsigned int round;
    unsigned int i;

    printf("Blusys i2c slave basic\n");
    printf("target: %s\n", blusys_target_name());
    printf("i2c_slave supported: %s\n",
           blusys_target_supports(BLUSYS_FEATURE_I2C_SLAVE) ? "yes" : "no");

    if (!blusys_target_supports(BLUSYS_FEATURE_I2C_SLAVE)) {
        printf("i2c slave not supported on this target\n");
        return;
    }

    printf("sda: %d  scl: %d  addr: 0x%02x\n",
           BLUSYS_I2C_SLAVE_BASIC_SDA_PIN, BLUSYS_I2C_SLAVE_BASIC_SCL_PIN,
           (unsigned int) BLUSYS_I2C_SLAVE_BASIC_ADDR);

    err = blusys_i2c_slave_open(BLUSYS_I2C_SLAVE_BASIC_SDA_PIN,
                                BLUSYS_I2C_SLAVE_BASIC_SCL_PIN,
                                BLUSYS_I2C_SLAVE_BASIC_ADDR,
                                BLUSYS_I2C_SLAVE_BASIC_TX_BUF,
                                &slave);
    if (err != BLUSYS_OK) {
        printf("open error: %s\n", blusys_err_string(err));
        return;
    }

    printf("waiting for I2C master to send data\n");

    for (round = 0u; round < BLUSYS_I2C_SLAVE_BASIC_ROUNDS; ++round) {
        memset(rx_buf, 0, sizeof(rx_buf));

        err = blusys_i2c_slave_receive(slave, rx_buf, BLUSYS_I2C_SLAVE_BASIC_RX_SIZE,
                                       &received, BLUSYS_I2C_SLAVE_BASIC_TIMEOUT_MS);
        if (err == BLUSYS_ERR_TIMEOUT) {
            printf("round %u: timeout — no data received\n", round);
            continue;
        }
        if (err != BLUSYS_OK) {
            printf("round %u receive error: %s\n", round, blusys_err_string(err));
            break;
        }

        printf("round %u: received %u bytes:", round, (unsigned int) received);
        for (i = 0u; i < received; ++i) {
            printf(" %02x", (unsigned int) rx_buf[i]);
        }
        printf("\n");

        /* echo back the same bytes */
        memcpy(tx_buf, rx_buf, received);
        err = blusys_i2c_slave_transmit(slave, tx_buf, received,
                                        BLUSYS_I2C_SLAVE_BASIC_TIMEOUT_MS);
        if (err != BLUSYS_OK) {
            printf("transmit error: %s\n", blusys_err_string(err));
        }
    }

    blusys_i2c_slave_close(slave);
    printf("done\n");
}

#else /* !CONFIG_PERIPHERAL_LAB_SCENARIO_I2C_SLAVE */
void run_periph_i2c_slave(void) {}
#endif /* CONFIG_PERIPHERAL_LAB_SCENARIO_I2C_SLAVE */
