#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "blusys/espnow.h"

/* Broadcast address — every ESP-NOW node accepts frames sent here */
static const uint8_t BROADCAST_ADDR[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

static void recv_cb(const uint8_t peer_addr[6], const uint8_t *data,
                    size_t len, void *user_ctx)
{
    printf("[espnow] recv from %02x:%02x:%02x:%02x:%02x:%02x  len=%u  \"%.*s\"\n",
           peer_addr[0], peer_addr[1], peer_addr[2],
           peer_addr[3], peer_addr[4], peer_addr[5],
           (unsigned)len, (int)len, (const char *)data);
}

static void send_cb(const uint8_t peer_addr[6], bool success, void *user_ctx)
{
    printf("[espnow] send to %02x:%02x:%02x:%02x:%02x:%02x  %s\n",
           peer_addr[0], peer_addr[1], peer_addr[2],
           peer_addr[3], peer_addr[4], peer_addr[5],
           success ? "OK" : "FAIL");
}

void app_main(void)
{
    blusys_espnow_t *handle = NULL;

    blusys_espnow_config_t cfg = {
        .channel        = 0,
        .recv_cb        = recv_cb,
        .recv_user_ctx  = NULL,
        .send_cb        = send_cb,
        .send_user_ctx  = NULL,
    };

    blusys_err_t err = blusys_espnow_open(&cfg, &handle);
    if (err != BLUSYS_OK) {
        printf("[espnow] open failed: %s\n", blusys_err_string(err));
        return;
    }
    printf("[espnow] open OK\n");

    /* Register the broadcast peer so we can transmit to it */
    blusys_espnow_peer_t peer = {
        .channel = 0,
        .encrypt = false,
    };
    memcpy(peer.addr, BROADCAST_ADDR, 6);

    err = blusys_espnow_add_peer(handle, &peer);
    if (err != BLUSYS_OK) {
        printf("[espnow] add_peer failed: %s\n", blusys_err_string(err));
        blusys_espnow_close(handle);
        return;
    }
    printf("[espnow] broadcast peer added\n");

    /* Send 5 messages, one every 2 s */
    for (int i = 1; i <= 5; i++) {
        char msg[32];
        int len = snprintf(msg, sizeof(msg), "hello from espnow #%d", i);

        err = blusys_espnow_send(handle, BROADCAST_ADDR,
                                 (const uint8_t *)msg, (size_t)len);
        printf("[espnow] send #%d: %s\n", i,
               err == BLUSYS_OK ? "queued" : blusys_err_string(err));

        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    err = blusys_espnow_remove_peer(handle, BROADCAST_ADDR);
    printf("[espnow] remove_peer: %s\n",
           err == BLUSYS_OK ? "OK" : blusys_err_string(err));

    err = blusys_espnow_close(handle);
    printf("[espnow] close: %s\n",
           err == BLUSYS_OK ? "OK" : blusys_err_string(err));
}
