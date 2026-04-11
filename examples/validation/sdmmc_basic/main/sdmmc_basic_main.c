#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "sdkconfig.h"

#include "blusys/blusys.h"

#define BLUSYS_SDMMC_BASIC_SLOT      CONFIG_BLUSYS_SDMMC_BASIC_SLOT
#define BLUSYS_SDMMC_BASIC_CLK_PIN   CONFIG_BLUSYS_SDMMC_BASIC_CLK_PIN
#define BLUSYS_SDMMC_BASIC_CMD_PIN   CONFIG_BLUSYS_SDMMC_BASIC_CMD_PIN
#define BLUSYS_SDMMC_BASIC_D0_PIN    CONFIG_BLUSYS_SDMMC_BASIC_D0_PIN
#define BLUSYS_SDMMC_BASIC_BUS_WIDTH CONFIG_BLUSYS_SDMMC_BASIC_BUS_WIDTH
#define BLUSYS_SDMMC_BASIC_FREQ_HZ   ((uint32_t) CONFIG_BLUSYS_SDMMC_BASIC_FREQ_HZ)

#if CONFIG_BLUSYS_SDMMC_BASIC_BUS_WIDTH == 4
#define BLUSYS_SDMMC_BASIC_D1_PIN CONFIG_BLUSYS_SDMMC_BASIC_D1_PIN
#define BLUSYS_SDMMC_BASIC_D2_PIN CONFIG_BLUSYS_SDMMC_BASIC_D2_PIN
#define BLUSYS_SDMMC_BASIC_D3_PIN CONFIG_BLUSYS_SDMMC_BASIC_D3_PIN
#else
#define BLUSYS_SDMMC_BASIC_D1_PIN (-1)
#define BLUSYS_SDMMC_BASIC_D2_PIN (-1)
#define BLUSYS_SDMMC_BASIC_D3_PIN (-1)
#endif

#define BLUSYS_SDMMC_BASIC_BLOCK_SIZE 512u
#define BLUSYS_SDMMC_BASIC_TEST_BLOCK 0u
#define BLUSYS_SDMMC_BASIC_TIMEOUT_MS 5000

void app_main(void)
{
    blusys_sdmmc_t *sdmmc = NULL;
    blusys_err_t err;
    uint8_t write_buf[BLUSYS_SDMMC_BASIC_BLOCK_SIZE];
    uint8_t read_buf[BLUSYS_SDMMC_BASIC_BLOCK_SIZE];

    printf("Blusys sdmmc basic\n");
    printf("target: %s\n", blusys_target_name());
    printf("sdmmc supported: %s\n",
           blusys_target_supports(BLUSYS_FEATURE_SDMMC) ? "yes" : "no");
    printf("slot: %d\n", BLUSYS_SDMMC_BASIC_SLOT);
    printf("clk: %d  cmd: %d  d0: %d  bus_width: %d  freq: %lu Hz\n",
           BLUSYS_SDMMC_BASIC_CLK_PIN,
           BLUSYS_SDMMC_BASIC_CMD_PIN,
           BLUSYS_SDMMC_BASIC_D0_PIN,
           BLUSYS_SDMMC_BASIC_BUS_WIDTH,
           (unsigned long) BLUSYS_SDMMC_BASIC_FREQ_HZ);

    err = blusys_sdmmc_open(BLUSYS_SDMMC_BASIC_SLOT,
                            BLUSYS_SDMMC_BASIC_CLK_PIN,
                            BLUSYS_SDMMC_BASIC_CMD_PIN,
                            BLUSYS_SDMMC_BASIC_D0_PIN,
                            BLUSYS_SDMMC_BASIC_D1_PIN,
                            BLUSYS_SDMMC_BASIC_D2_PIN,
                            BLUSYS_SDMMC_BASIC_D3_PIN,
                            BLUSYS_SDMMC_BASIC_BUS_WIDTH,
                            BLUSYS_SDMMC_BASIC_FREQ_HZ,
                            &sdmmc);
    if (err != BLUSYS_OK) {
        printf("open error: %s\n", blusys_err_string(err));
        return;
    }
    printf("sdmmc_open: ok\n");

    for (uint32_t i = 0u; i < BLUSYS_SDMMC_BASIC_BLOCK_SIZE; i++) {
        write_buf[i] = (uint8_t) (i & 0xFFu);
    }

    err = blusys_sdmmc_write_blocks(sdmmc,
                                    BLUSYS_SDMMC_BASIC_TEST_BLOCK,
                                    write_buf,
                                    1u,
                                    BLUSYS_SDMMC_BASIC_TIMEOUT_MS);
    if (err != BLUSYS_OK) {
        printf("write_blocks error: %s\n", blusys_err_string(err));
        blusys_sdmmc_close(sdmmc);
        return;
    }
    printf("write_blocks: ok\n");

    memset(read_buf, 0, sizeof(read_buf));

    err = blusys_sdmmc_read_blocks(sdmmc,
                                   BLUSYS_SDMMC_BASIC_TEST_BLOCK,
                                   read_buf,
                                   1u,
                                   BLUSYS_SDMMC_BASIC_TIMEOUT_MS);
    if (err != BLUSYS_OK) {
        printf("read_blocks error: %s\n", blusys_err_string(err));
        blusys_sdmmc_close(sdmmc);
        return;
    }
    printf("read_blocks: ok\n");

    if (memcmp(write_buf, read_buf, BLUSYS_SDMMC_BASIC_BLOCK_SIZE) == 0) {
        printf("verify: PASS\n");
    } else {
        printf("verify: FAIL\n");
    }

    err = blusys_sdmmc_close(sdmmc);
    if (err != BLUSYS_OK) {
        printf("close error: %s\n", blusys_err_string(err));
        return;
    }
    printf("sdmmc_close: ok\n");
}
