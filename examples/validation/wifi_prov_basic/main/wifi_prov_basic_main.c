#include <stdio.h>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"

#include "sdkconfig.h"
#include "blusys/blusys_services.h"

#define PROV_DONE_BIT BIT0
#define PROV_FAIL_BIT BIT1

static EventGroupHandle_t s_event_group;

static void prov_event_cb(blusys_wifi_prov_event_t event,
                           const blusys_wifi_prov_credentials_t *creds,
                           void *user_ctx)
{
    (void)user_ctx;

    switch (event) {
        case BLUSYS_WIFI_PROV_EVENT_STARTED:
            printf("Provisioning started — waiting for credentials...\n");
            break;

        case BLUSYS_WIFI_PROV_EVENT_CREDENTIALS_RECEIVED:
            printf("Credentials received: SSID='%s'\n", creds->ssid);
            break;

        case BLUSYS_WIFI_PROV_EVENT_SUCCESS:
            printf("Provisioning succeeded — device is connected.\n");
            xEventGroupSetBits(s_event_group, PROV_DONE_BIT);
            break;

        case BLUSYS_WIFI_PROV_EVENT_FAILED:
            printf("Provisioning failed — invalid credentials.\n");
            xEventGroupSetBits(s_event_group, PROV_FAIL_BIT);
            break;
    }
}

void app_main(void)
{
    blusys_err_t err;

    s_event_group = xEventGroupCreate();

#if CONFIG_PROV_TRANSPORT_BLE
    blusys_wifi_prov_transport_t transport = BLUSYS_WIFI_PROV_TRANSPORT_BLE;
#else
    blusys_wifi_prov_transport_t transport = BLUSYS_WIFI_PROV_TRANSPORT_SOFTAP;
#endif

    blusys_wifi_prov_config_t cfg = {
        .transport    = transport,
        .service_name = CONFIG_PROV_SERVICE_NAME,
        .pop          = CONFIG_PROV_POP[0] ? CONFIG_PROV_POP : NULL,
        .service_key  = CONFIG_PROV_SERVICE_KEY[0] ? CONFIG_PROV_SERVICE_KEY : NULL,
        .on_event     = prov_event_cb,
        .user_ctx     = NULL,
    };

    blusys_wifi_prov_t *prov;
    err = blusys_wifi_prov_open(&cfg, &prov);
    if (err != BLUSYS_OK) {
        printf("wifi_prov_open failed: %s\n", blusys_err_string(err));
        vEventGroupDelete(s_event_group);
        return;
    }

    if (blusys_wifi_prov_is_provisioned()) {
        printf("Device is already provisioned.\n");
        printf("Call blusys_wifi_prov_reset() to erase credentials and re-provision.\n");
        blusys_wifi_prov_close(prov);
        vEventGroupDelete(s_event_group);
        return;
    }

    /* Print QR payload for use with the ESP Provisioning mobile app */
    char qr[256];
    err = blusys_wifi_prov_get_qr_payload(prov, qr, sizeof(qr));
    if (err == BLUSYS_OK) {
        printf("Scan this payload with the ESP Provisioning app:\n%s\n", qr);
    }

    err = blusys_wifi_prov_start(prov);
    if (err != BLUSYS_OK) {
        printf("wifi_prov_start failed: %s\n", blusys_err_string(err));
        blusys_wifi_prov_close(prov);
        return;
    }

    /* Block until provisioning completes or fails */
    EventBits_t bits = xEventGroupWaitBits(s_event_group,
                                           PROV_DONE_BIT | PROV_FAIL_BIT,
                                           pdFALSE, pdFALSE,
                                           portMAX_DELAY);

    if (bits & PROV_DONE_BIT) {
        printf("Provisioning complete. Credentials stored to NVS.\n");
    } else {
        printf("Provisioning failed.\n");
    }

    blusys_wifi_prov_stop(prov);
    blusys_wifi_prov_close(prov);

    vEventGroupDelete(s_event_group);
}
