#include "blusys/hal/sdmmc.h"

#include <stdbool.h>
#include <stdlib.h>

#include "soc/soc_caps.h"

#if SOC_SDMMC_HOST_SUPPORTED

#include "blusys/hal/internal/esp_err_shim.h"
#include "blusys/hal/internal/lock.h"
#include "blusys/hal/internal/timeout.h"

#include "driver/gpio.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"

struct blusys_sdmmc {
    sdmmc_host_t host;
    sdmmc_card_t *card;
    int slot;
    bool closing;
    blusys_lock_t lock;
};

static bool blusys_sdmmc_is_valid_slot(int slot)
{
    return (slot >= 0) && (slot < SOC_SDMMC_NUM_SLOTS);
}

static bool blusys_sdmmc_is_valid_pin(int pin)
{
    return GPIO_IS_VALID_GPIO(pin);
}

static bool blusys_sdmmc_is_valid_bus_width(int bus_width)
{
    return (bus_width == 1) || (bus_width == 4);
}

blusys_err_t blusys_sdmmc_open(int slot,
                               int clk_pin,
                               int cmd_pin,
                               int d0_pin,
                               int d1_pin,
                               int d2_pin,
                               int d3_pin,
                               int bus_width,
                               uint32_t freq_hz,
                               blusys_sdmmc_t **out_sdmmc)
{
    blusys_sdmmc_t *sdmmc;
    blusys_err_t err;
    esp_err_t esp_err;

    if (!blusys_sdmmc_is_valid_slot(slot) ||
        !blusys_sdmmc_is_valid_pin(clk_pin) ||
        !blusys_sdmmc_is_valid_pin(cmd_pin) ||
        !blusys_sdmmc_is_valid_pin(d0_pin) ||
        !blusys_sdmmc_is_valid_bus_width(bus_width) ||
        (freq_hz == 0u) ||
        (out_sdmmc == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    if (bus_width == 4) {
        if (!blusys_sdmmc_is_valid_pin(d1_pin) ||
            !blusys_sdmmc_is_valid_pin(d2_pin) ||
            !blusys_sdmmc_is_valid_pin(d3_pin)) {
            return BLUSYS_ERR_INVALID_ARG;
        }
    }

    sdmmc = calloc(1, sizeof(*sdmmc));
    if (sdmmc == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    err = blusys_lock_init(&sdmmc->lock);
    if (err != BLUSYS_OK) {
        free(sdmmc);
        return err;
    }

    sdmmc->slot = slot;

    sdmmc_host_t host_config = SDMMC_HOST_DEFAULT();
    host_config.slot = slot;
    host_config.max_freq_khz = (int) (freq_hz / 1000u);

    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.clk = clk_pin;
    slot_config.cmd = cmd_pin;
    slot_config.d0 = d0_pin;
    slot_config.width = bus_width;

    if (bus_width == 4) {
        slot_config.d1 = d1_pin;
        slot_config.d2 = d2_pin;
        slot_config.d3 = d3_pin;
    }

    esp_err = sdmmc_host_init();
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        blusys_lock_deinit(&sdmmc->lock);
        free(sdmmc);
        return err;
    }

    esp_err = sdmmc_host_init_slot(slot, &slot_config);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        sdmmc_host_deinit();
        blusys_lock_deinit(&sdmmc->lock);
        free(sdmmc);
        return err;
    }

    sdmmc->card = calloc(1, sizeof(sdmmc_card_t));
    if (sdmmc->card == NULL) {
        sdmmc_host_deinit_slot(slot);
        blusys_lock_deinit(&sdmmc->lock);
        free(sdmmc);
        return BLUSYS_ERR_NO_MEM;
    }

    sdmmc->host = host_config;

    esp_err = sdmmc_card_init(&sdmmc->host, sdmmc->card);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        free(sdmmc->card);
        sdmmc_host_deinit_slot(slot);
        blusys_lock_deinit(&sdmmc->lock);
        free(sdmmc);
        return err;
    }

    *out_sdmmc = sdmmc;
    return BLUSYS_OK;
}

blusys_err_t blusys_sdmmc_close(blusys_sdmmc_t *sdmmc)
{
    blusys_err_t err;
    esp_err_t esp_err = ESP_OK;

    if (sdmmc == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&sdmmc->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    sdmmc->closing = true;

    esp_err = sdmmc_host_deinit_slot(sdmmc->slot);
    err = blusys_translate_esp_err(esp_err);

    if (err != BLUSYS_OK) {
        sdmmc->closing = false;
        blusys_lock_give(&sdmmc->lock);
        return err;
    }

    esp_err = sdmmc_host_deinit();
    if (esp_err != ESP_OK) {
        sdmmc->closing = false;
        blusys_lock_give(&sdmmc->lock);
        return blusys_translate_esp_err(esp_err);
    }

    free(sdmmc->card);
    sdmmc->card = NULL;

    blusys_lock_give(&sdmmc->lock);
    blusys_lock_deinit(&sdmmc->lock);
    free(sdmmc);
    return BLUSYS_OK;
}

blusys_err_t blusys_sdmmc_read_blocks(blusys_sdmmc_t *sdmmc,
                                      uint32_t start_block,
                                      void *buffer,
                                      uint32_t num_blocks,
                                      int timeout_ms)
{
    blusys_err_t err;
    esp_err_t esp_err;

    if ((sdmmc == NULL) || (buffer == NULL) || (num_blocks == 0u)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    if (!blusys_timeout_ms_is_valid(timeout_ms)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&sdmmc->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (sdmmc->closing) {
        blusys_lock_give(&sdmmc->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    esp_err = sdmmc_read_sectors(sdmmc->card, buffer, (size_t) start_block, (size_t) num_blocks);
    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&sdmmc->lock);
    return err;
}

blusys_err_t blusys_sdmmc_write_blocks(blusys_sdmmc_t *sdmmc,
                                       uint32_t start_block,
                                       const void *buffer,
                                       uint32_t num_blocks,
                                       int timeout_ms)
{
    blusys_err_t err;
    esp_err_t esp_err;

    if ((sdmmc == NULL) || (buffer == NULL) || (num_blocks == 0u)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    if (!blusys_timeout_ms_is_valid(timeout_ms)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&sdmmc->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    if (sdmmc->closing) {
        blusys_lock_give(&sdmmc->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    esp_err = sdmmc_write_sectors(sdmmc->card, buffer, (size_t) start_block, (size_t) num_blocks);
    err = blusys_translate_esp_err(esp_err);

    blusys_lock_give(&sdmmc->lock);
    return err;
}

#else /* !SOC_SDMMC_HOST_SUPPORTED */

blusys_err_t blusys_sdmmc_open(int slot,
                               int clk_pin,
                               int cmd_pin,
                               int d0_pin,
                               int d1_pin,
                               int d2_pin,
                               int d3_pin,
                               int bus_width,
                               uint32_t freq_hz,
                               blusys_sdmmc_t **out_sdmmc)
{
    (void) slot;
    (void) clk_pin;
    (void) cmd_pin;
    (void) d0_pin;
    (void) d1_pin;
    (void) d2_pin;
    (void) d3_pin;
    (void) bus_width;
    (void) freq_hz;
    (void) out_sdmmc;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_sdmmc_close(blusys_sdmmc_t *sdmmc)
{
    (void) sdmmc;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_sdmmc_read_blocks(blusys_sdmmc_t *sdmmc,
                                      uint32_t start_block,
                                      void *buffer,
                                      uint32_t num_blocks,
                                      int timeout_ms)
{
    (void) sdmmc;
    (void) start_block;
    (void) buffer;
    (void) num_blocks;
    (void) timeout_ms;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_sdmmc_write_blocks(blusys_sdmmc_t *sdmmc,
                                       uint32_t start_block,
                                       const void *buffer,
                                       uint32_t num_blocks,
                                       int timeout_ms)
{
    (void) sdmmc;
    (void) start_block;
    (void) buffer;
    (void) num_blocks;
    (void) timeout_ms;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

#endif /* SOC_SDMMC_HOST_SUPPORTED */
