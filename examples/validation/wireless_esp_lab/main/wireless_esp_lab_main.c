#include "sdkconfig.h"

void run_espnow_basic(void);
void run_wifi_mesh_basic(void);

void app_main(void)
{
#if CONFIG_WIRELESS_ESP_LAB_SCENARIO_ESPNOW_BASIC
    run_espnow_basic();
#elif CONFIG_WIRELESS_ESP_LAB_SCENARIO_WIFI_MESH_BASIC
    run_wifi_mesh_basic();
#endif
}
