#include "sdkconfig.h"

#if CONFIG_REF_HAL_REFERENCE_SCENARIO_SPI_LOOPBACK

#include <stdio.h>
#include <string.h>

#include <stdbool.h>
#include <stdint.h>


#include "blusys/blusys.h"

#define BLUSYS_SPI_LOOPBACK_BUS CONFIG_BLUSYS_SPI_LOOPBACK_BUS
#define BLUSYS_SPI_LOOPBACK_SCLK_PIN CONFIG_BLUSYS_SPI_LOOPBACK_SCLK_PIN
#define BLUSYS_SPI_LOOPBACK_MOSI_PIN CONFIG_BLUSYS_SPI_LOOPBACK_MOSI_PIN
#define BLUSYS_SPI_LOOPBACK_MISO_PIN CONFIG_BLUSYS_SPI_LOOPBACK_MISO_PIN
#define BLUSYS_SPI_LOOPBACK_CS_PIN CONFIG_BLUSYS_SPI_LOOPBACK_CS_PIN

#define BLUSYS_SPI_LOOPBACK_FREQ_HZ 1000000u

void run_spi_loopback(void)
{
    static const uint8_t tx_patterns[][8] = {
        {0x12u, 0x34u, 0x56u, 0x78u, 0x9au, 0xbcu, 0xdeu, 0xf0u},
        {0xffu, 0x00u, 0xa5u, 0x5au, 0x3cu, 0xc3u, 0x69u, 0x96u},
        {0x81u, 0x7eu, 0x24u, 0xdbu, 0x42u, 0xbdu, 0x18u, 0xe7u},
        {0x00u, 0xffu, 0x11u, 0xeeu, 0x22u, 0xddu, 0x44u, 0xbbu},
    };
    uint8_t rx_data[sizeof(tx_patterns[0])] = {0};
    blusys_spi_t *spi = NULL;
    blusys_err_t err;
    bool all_ok = true;
    size_t pattern_index;

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

    for (pattern_index = 0u; pattern_index < (sizeof(tx_patterns) / sizeof(tx_patterns[0])); ++pattern_index) {
        const uint8_t *tx_data = tx_patterns[pattern_index];

        memset(rx_data, 0, sizeof(rx_data));

        err = blusys_spi_transfer(spi, tx_data, rx_data, sizeof(rx_data));
        if (err != BLUSYS_OK) {
            printf("transfer error: %s\n", blusys_err_string(err));
            blusys_spi_close(spi);
            return;
        }

        printf("pattern_%u tx: %02x %02x %02x %02x %02x %02x %02x %02x\n",
               (unsigned int) pattern_index,
               (unsigned int) tx_data[0],
               (unsigned int) tx_data[1],
               (unsigned int) tx_data[2],
               (unsigned int) tx_data[3],
               (unsigned int) tx_data[4],
               (unsigned int) tx_data[5],
               (unsigned int) tx_data[6],
               (unsigned int) tx_data[7]);
        printf("pattern_%u rx: %02x %02x %02x %02x %02x %02x %02x %02x\n",
               (unsigned int) pattern_index,
               (unsigned int) rx_data[0],
               (unsigned int) rx_data[1],
               (unsigned int) rx_data[2],
               (unsigned int) rx_data[3],
               (unsigned int) rx_data[4],
               (unsigned int) rx_data[5],
               (unsigned int) rx_data[6],
               (unsigned int) rx_data[7]);

        if (memcmp(tx_data, rx_data, sizeof(rx_data)) != 0) {
            all_ok = false;
        }
    }

    printf("loopback_result: %s\n", all_ok ? "ok" : "mismatch");

    err = blusys_spi_close(spi);
    if (err != BLUSYS_OK) {
        printf("close error: %s\n", blusys_err_string(err));
    }
}


#else

void run_spi_loopback(void) {}

#endif /* CONFIG_REF_HAL_REFERENCE_SCENARIO_SPI_LOOPBACK */
