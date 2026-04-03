#include <stdio.h>
#include <string.h>

#include <stdint.h>

#include "sdkconfig.h"

#include "blusys/blusys.h"

#define BLUSYS_SPI_LOOPBACK_BUS CONFIG_BLUSYS_SPI_LOOPBACK_BUS
#define BLUSYS_SPI_LOOPBACK_SCLK_PIN CONFIG_BLUSYS_SPI_LOOPBACK_SCLK_PIN
#define BLUSYS_SPI_LOOPBACK_MOSI_PIN CONFIG_BLUSYS_SPI_LOOPBACK_MOSI_PIN
#define BLUSYS_SPI_LOOPBACK_MISO_PIN CONFIG_BLUSYS_SPI_LOOPBACK_MISO_PIN
#define BLUSYS_SPI_LOOPBACK_CS_PIN CONFIG_BLUSYS_SPI_LOOPBACK_CS_PIN

#define BLUSYS_SPI_LOOPBACK_FREQ_HZ 1000000u

void app_main(void)
{
    static const uint8_t tx_data[] = {0x12u, 0x34u, 0x56u, 0x78u};
    uint8_t rx_data[sizeof(tx_data)] = {0};
    blusys_spi_t *spi = NULL;
    blusys_err_t err;

    printf("Blusys spi loopback\n");
    printf("target: %s\n", blusys_target_name());
    printf("bus: %d\n", BLUSYS_SPI_LOOPBACK_BUS);
    printf("sclk_pin: %d\n", BLUSYS_SPI_LOOPBACK_SCLK_PIN);
    printf("mosi_pin: %d\n", BLUSYS_SPI_LOOPBACK_MOSI_PIN);
    printf("miso_pin: %d\n", BLUSYS_SPI_LOOPBACK_MISO_PIN);
    printf("cs_pin: %d\n", BLUSYS_SPI_LOOPBACK_CS_PIN);
    printf("wire mosi_pin to miso_pin before running this example\n");
    printf("set board-safe SPI pins in menuconfig if these defaults do not match your board\n");

    err = blusys_spi_open(BLUSYS_SPI_LOOPBACK_BUS,
                          BLUSYS_SPI_LOOPBACK_SCLK_PIN,
                          BLUSYS_SPI_LOOPBACK_MOSI_PIN,
                          BLUSYS_SPI_LOOPBACK_MISO_PIN,
                          BLUSYS_SPI_LOOPBACK_CS_PIN,
                          BLUSYS_SPI_LOOPBACK_FREQ_HZ,
                          &spi);
    if (err != BLUSYS_OK) {
        printf("open error: %s\n", blusys_err_string(err));
        return;
    }

    err = blusys_spi_transfer(spi, tx_data, rx_data, sizeof(tx_data));
    if (err != BLUSYS_OK) {
        printf("transfer error: %s\n", blusys_err_string(err));
        blusys_spi_close(spi);
        return;
    }

    printf("tx: %02x %02x %02x %02x\n",
           (unsigned int) tx_data[0],
           (unsigned int) tx_data[1],
           (unsigned int) tx_data[2],
           (unsigned int) tx_data[3]);
    printf("rx: %02x %02x %02x %02x\n",
           (unsigned int) rx_data[0],
           (unsigned int) rx_data[1],
           (unsigned int) rx_data[2],
           (unsigned int) rx_data[3]);
    printf("loopback_result: %s\n", (memcmp(tx_data, rx_data, sizeof(tx_data)) == 0) ? "ok" : "mismatch");

    err = blusys_spi_close(spi);
    if (err != BLUSYS_OK) {
        printf("close error: %s\n", blusys_err_string(err));
    }
}
