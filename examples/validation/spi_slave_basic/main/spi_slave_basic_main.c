#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "blusys/blusys.h"

#define BLUSYS_SPI_SLAVE_BASIC_MOSI_PIN CONFIG_BLUSYS_SPI_SLAVE_BASIC_MOSI_PIN
#define BLUSYS_SPI_SLAVE_BASIC_MISO_PIN CONFIG_BLUSYS_SPI_SLAVE_BASIC_MISO_PIN
#define BLUSYS_SPI_SLAVE_BASIC_CLK_PIN  CONFIG_BLUSYS_SPI_SLAVE_BASIC_CLK_PIN
#define BLUSYS_SPI_SLAVE_BASIC_CS_PIN   CONFIG_BLUSYS_SPI_SLAVE_BASIC_CS_PIN

#define BLUSYS_SPI_SLAVE_BASIC_BUF_SIZE  16u
#define BLUSYS_SPI_SLAVE_BASIC_ROUNDS    5u
#define BLUSYS_SPI_SLAVE_BASIC_TIMEOUT_MS 5000

void app_main(void)
{
    blusys_spi_slave_t *slave = NULL;
    blusys_err_t err;
    static uint8_t tx_buf[BLUSYS_SPI_SLAVE_BASIC_BUF_SIZE];
    static uint8_t rx_buf[BLUSYS_SPI_SLAVE_BASIC_BUF_SIZE];
    unsigned int i;
    unsigned int j;

    printf("Blusys spi slave basic\n");
    printf("target: %s\n", blusys_target_name());
    printf("spi_slave supported: %s\n",
           blusys_target_supports(BLUSYS_FEATURE_SPI_SLAVE) ? "yes" : "no");

    if (!blusys_target_supports(BLUSYS_FEATURE_SPI_SLAVE)) {
        printf("spi slave not supported on this target\n");
        return;
    }

    printf("mosi: %d  miso: %d  clk: %d  cs: %d\n",
           BLUSYS_SPI_SLAVE_BASIC_MOSI_PIN, BLUSYS_SPI_SLAVE_BASIC_MISO_PIN,
           BLUSYS_SPI_SLAVE_BASIC_CLK_PIN, BLUSYS_SPI_SLAVE_BASIC_CS_PIN);

    err = blusys_spi_slave_open(BLUSYS_SPI_SLAVE_BASIC_MOSI_PIN,
                                BLUSYS_SPI_SLAVE_BASIC_MISO_PIN,
                                BLUSYS_SPI_SLAVE_BASIC_CLK_PIN,
                                BLUSYS_SPI_SLAVE_BASIC_CS_PIN,
                                0u,
                                &slave);
    if (err != BLUSYS_OK) {
        printf("open error: %s\n", blusys_err_string(err));
        return;
    }

    printf("waiting for SPI master to initiate transfers\n");

    for (i = 0u; i < BLUSYS_SPI_SLAVE_BASIC_ROUNDS; ++i) {
        /* prepare TX: send round index in each byte */
        for (j = 0u; j < BLUSYS_SPI_SLAVE_BASIC_BUF_SIZE; ++j) {
            tx_buf[j] = (uint8_t) (i * 16u + j);
        }
        memset(rx_buf, 0, sizeof(rx_buf));

        err = blusys_spi_slave_transfer(slave, tx_buf, rx_buf,
                                        BLUSYS_SPI_SLAVE_BASIC_BUF_SIZE,
                                        BLUSYS_SPI_SLAVE_BASIC_TIMEOUT_MS);
        if (err != BLUSYS_OK) {
            printf("transfer %u error: %s\n", i, blusys_err_string(err));
            break;
        }
        printf("transfer %u: rx[0]=%u rx[1]=%u ...\n",
               i, (unsigned int) rx_buf[0], (unsigned int) rx_buf[1]);
    }

    blusys_spi_slave_close(slave);
    printf("done\n");
}
