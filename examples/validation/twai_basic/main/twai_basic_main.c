#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "sdkconfig.h"

#include "blusys/blusys.h"

#define BLUSYS_TWAI_BASIC_TX_PIN CONFIG_BLUSYS_TWAI_BASIC_TX_PIN
#define BLUSYS_TWAI_BASIC_RX_PIN CONFIG_BLUSYS_TWAI_BASIC_RX_PIN
#define BLUSYS_TWAI_BASIC_BITRATE CONFIG_BLUSYS_TWAI_BASIC_BITRATE

static volatile bool rx_pending;
static volatile uint32_t rx_id;
static volatile bool rx_remote_frame;
static volatile uint8_t rx_dlc;
static volatile size_t rx_data_len;
static volatile uint8_t rx_data[8];

static bool on_rx_frame(blusys_twai_t *twai, const blusys_twai_frame_t *frame, void *user_ctx)
{
    size_t index;

    (void) twai;
    (void) user_ctx;

    rx_id = frame->id;
    rx_remote_frame = frame->remote_frame;
    rx_dlc = frame->dlc;
    rx_data_len = frame->data_len;
    for (index = 0u; index < frame->data_len; ++index) {
        rx_data[index] = frame->data[index];
    }
    rx_pending = true;

    return false;
}

void app_main(void)
{
    blusys_twai_t *twai = NULL;
    blusys_err_t err;
    uint8_t counter = 0u;

    printf("Blusys twai basic\n");
    printf("target: %s\n", blusys_target_name());
    printf("tx_pin: %d\n", BLUSYS_TWAI_BASIC_TX_PIN);
    printf("rx_pin: %d\n", BLUSYS_TWAI_BASIC_RX_PIN);
    printf("bitrate: %d\n", BLUSYS_TWAI_BASIC_BITRATE);
    printf("connect an external TWAI transceiver and another active node on the bus\n");

    err = blusys_twai_open(BLUSYS_TWAI_BASIC_TX_PIN,
                           BLUSYS_TWAI_BASIC_RX_PIN,
                           BLUSYS_TWAI_BASIC_BITRATE,
                           &twai);
    if (err != BLUSYS_OK) {
        printf("open error: %s\n", blusys_err_string(err));
        return;
    }

    err = blusys_twai_set_rx_callback(twai, on_rx_frame, NULL);
    if (err != BLUSYS_OK) {
        printf("set_rx_callback error: %s\n", blusys_err_string(err));
        blusys_twai_close(twai);
        return;
    }

    err = blusys_twai_start(twai);
    if (err != BLUSYS_OK) {
        printf("start error: %s\n", blusys_err_string(err));
        blusys_twai_close(twai);
        return;
    }

    while (true) {
        blusys_twai_frame_t frame = {
            .id = 0x123u,
            .remote_frame = false,
            .dlc = 4u,
            .data = {counter, (uint8_t) (counter + 1u), 0x42u, 0x99u},
            .data_len = 4u,
        };

        err = blusys_twai_write(twai, &frame, 1000);
        if (err != BLUSYS_OK) {
            printf("write error: %s\n", blusys_err_string(err));
            break;
        }

        printf("tx id=0x%03x len=%u counter=%u\n", (unsigned int) frame.id, (unsigned int) frame.dlc, counter);
        if (rx_pending) {
            size_t index;

            printf("rx id=0x%03x rtr=%d dlc=%u len=%u",
                   (unsigned int) rx_id,
                   rx_remote_frame ? 1 : 0,
                   (unsigned int) rx_dlc,
                   (unsigned int) rx_data_len);
            for (index = 0u; index < rx_data_len; ++index) {
                printf(" %02x", rx_data[index]);
            }
            printf("\n");
            rx_pending = false;
        }

        counter += 1u;
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

    blusys_twai_stop(twai);
    blusys_twai_close(twai);
}
