#include "sdkconfig.h"

void run_wifi_connect(void);
void run_http_client_basic(void);
void run_mqtt_basic(void);

void app_main(void)
{
#if CONFIG_CONNECTIVITY_SCENARIO_WIFI_CONNECT
    run_wifi_connect();
#elif CONFIG_CONNECTIVITY_SCENARIO_HTTP_CLIENT
    run_http_client_basic();
#elif CONFIG_CONNECTIVITY_SCENARIO_MQTT_BASIC
    run_mqtt_basic();
#endif
}
