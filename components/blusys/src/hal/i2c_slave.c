#include "blusys/hal/i2c_slave.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#include "soc/soc_caps.h"

#if SOC_I2C_SUPPORT_SLAVE

#include "blusys/hal/internal/esp_err_shim.h"
#include "blusys/hal/internal/lock.h"
#include "blusys/hal/internal/timeout.h"

#include "driver/gpio.h"
#include "driver/i2c_slave.h"

#include "esp_attr.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

struct blusys_i2c_slave {
    i2c_slave_dev_handle_t handle;
    size_t                 rx_requested;   /* byte count requested to i2c_slave_receive() */
    size_t                 rx_count;       /* bytes actually received (set by callback) */
    TaskHandle_t           waiting_task;   /* notified by ISR callback */
    blusys_lock_t          lock;
};

static IRAM_ATTR bool blusys_i2c_slave_rx_done_cb(i2c_slave_dev_handle_t i2c_slave,
                                                   const i2c_slave_rx_done_event_data_t *edata,
                                                   void *user_ctx)
{
    blusys_i2c_slave_t *h = (blusys_i2c_slave_t *) user_ctx;
    BaseType_t higher_prio_woken = pdFALSE;

    (void) i2c_slave;
    (void) edata;

    /* V1 driver: buffer_size is not in the event data — use the count stored at receive() time */
    h->rx_count = h->rx_requested;

    if (h->waiting_task != NULL) {
        vTaskNotifyGiveFromISR(h->waiting_task, &higher_prio_woken);
        h->waiting_task = NULL;
    }

    return higher_prio_woken == pdTRUE;
}

blusys_err_t blusys_i2c_slave_open(int sda_pin,
                                    int scl_pin,
                                    uint16_t addr,
                                    uint32_t tx_buf_size,
                                    blusys_i2c_slave_t **out_slave)
{
    blusys_i2c_slave_t *h;
    blusys_err_t err;
    esp_err_t esp_err;

    if (!GPIO_IS_VALID_GPIO(sda_pin) ||
        !GPIO_IS_VALID_GPIO(scl_pin) ||
        (tx_buf_size == 0u) ||
        (out_slave == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    h = calloc(1, sizeof(*h));
    if (h == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    err = blusys_lock_init(&h->lock);
    if (err != BLUSYS_OK) {
        free(h);
        return err;
    }

    i2c_slave_config_t cfg = {
        .i2c_port      = -1,   /* auto-select */
        .sda_io_num    = (gpio_num_t) sda_pin,
        .scl_io_num    = (gpio_num_t) scl_pin,
        .clk_source    = I2C_CLK_SRC_DEFAULT,
        .send_buf_depth = tx_buf_size,
        .slave_addr    = addr,
        .addr_bit_len  = I2C_ADDR_BIT_LEN_7,
        .intr_priority = 0,
    };
    esp_err = i2c_new_slave_device(&cfg, &h->handle);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        blusys_lock_deinit(&h->lock);
        free(h);
        return err;
    }

    i2c_slave_event_callbacks_t cbs = {
        .on_recv_done = blusys_i2c_slave_rx_done_cb,
    };
    esp_err = i2c_slave_register_event_callbacks(h->handle, &cbs, h);
    if (esp_err != ESP_OK) {
        err = blusys_translate_esp_err(esp_err);
        i2c_del_slave_device(h->handle);
        blusys_lock_deinit(&h->lock);
        free(h);
        return err;
    }

    *out_slave = h;
    return BLUSYS_OK;
}

blusys_err_t blusys_i2c_slave_close(blusys_i2c_slave_t *slave)
{
    blusys_err_t err;

    if (slave == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&slave->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_translate_esp_err(i2c_del_slave_device(slave->handle));

    blusys_lock_give(&slave->lock);

    if (err == BLUSYS_OK) {
        blusys_lock_deinit(&slave->lock);
        free(slave);
    }

    return err;
}

blusys_err_t blusys_i2c_slave_receive(blusys_i2c_slave_t *slave,
                                       uint8_t *buf,
                                       size_t size,
                                       size_t *bytes_received,
                                       int timeout_ms)
{
    blusys_err_t err;
    esp_err_t esp_err;
    uint32_t notify_val;

    if ((slave == NULL) || (buf == NULL) || (size == 0u) ||
        (bytes_received == NULL) || !blusys_timeout_ms_is_valid(timeout_ms)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&slave->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    /* Reject concurrent receives — waiting_task serves as the busy indicator. */
    if (slave->waiting_task != NULL) {
        blusys_lock_give(&slave->lock);
        return BLUSYS_ERR_INVALID_STATE;
    }

    slave->rx_count     = 0u;
    slave->rx_requested = size;
    slave->waiting_task = xTaskGetCurrentTaskHandle();

    esp_err = i2c_slave_receive(slave->handle, buf, size);
    if (esp_err != ESP_OK) {
        slave->waiting_task = NULL;
        blusys_lock_give(&slave->lock);
        return blusys_translate_esp_err(esp_err);
    }

    /* Release lock before blocking — the ISR notifies via task notification,
       not through the blusys_lock.  waiting_task prevents a second caller
       from starting another receive while we are blocked. */
    blusys_lock_give(&slave->lock);

    notify_val = ulTaskNotifyTake(pdTRUE,
                                  (timeout_ms < 0) ? portMAX_DELAY
                                                   : (TickType_t) pdMS_TO_TICKS(timeout_ms));
    if (notify_val == 0u) {
        slave->waiting_task = NULL;
        return BLUSYS_ERR_TIMEOUT;
    }

    *bytes_received = slave->rx_count;

    return BLUSYS_OK;
}

blusys_err_t blusys_i2c_slave_transmit(blusys_i2c_slave_t *slave,
                                        const uint8_t *buf,
                                        size_t size,
                                        int timeout_ms)
{
    blusys_err_t err;

    if ((slave == NULL) || (buf == NULL) || (size == 0u) ||
        !blusys_timeout_ms_is_valid(timeout_ms)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    err = blusys_lock_take(&slave->lock, BLUSYS_LOCK_WAIT_FOREVER);
    if (err != BLUSYS_OK) {
        return err;
    }

    err = blusys_translate_esp_err(
        i2c_slave_transmit(slave->handle, buf, (int) size,
                           (timeout_ms < 0) ? -1 : timeout_ms));

    blusys_lock_give(&slave->lock);
    return err;
}

#else

blusys_err_t blusys_i2c_slave_open(int sda_pin,
                                    int scl_pin,
                                    uint16_t addr,
                                    uint32_t tx_buf_size,
                                    blusys_i2c_slave_t **out_slave)
{
    (void) sda_pin;
    (void) scl_pin;
    (void) addr;
    (void) tx_buf_size;
    (void) out_slave;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_i2c_slave_close(blusys_i2c_slave_t *slave)
{
    (void) slave;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_i2c_slave_receive(blusys_i2c_slave_t *slave,
                                       uint8_t *buf,
                                       size_t size,
                                       size_t *bytes_received,
                                       int timeout_ms)
{
    (void) slave;
    (void) buf;
    (void) size;
    (void) bytes_received;
    (void) timeout_ms;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_i2c_slave_transmit(blusys_i2c_slave_t *slave,
                                        const uint8_t *buf,
                                        size_t size,
                                        int timeout_ms)
{
    (void) slave;
    (void) buf;
    (void) size;
    (void) timeout_ms;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

#endif
