#include "sdkconfig.h"

void run_net_http_server(void);
void run_net_ws_client(void);
void run_net_mdns(void);
void run_net_sntp(void);
void run_net_ota(void);
void run_net_wifi_prov(void);
void run_net_local_ctrl(void);

void app_main(void)
{
#if CONFIG_NETWORK_SERVICES_SCENARIO_HTTP_SERVER
    run_net_http_server();
#elif CONFIG_NETWORK_SERVICES_SCENARIO_WS_CLIENT
    run_net_ws_client();
#elif CONFIG_NETWORK_SERVICES_SCENARIO_MDNS
    run_net_mdns();
#elif CONFIG_NETWORK_SERVICES_SCENARIO_SNTP
    run_net_sntp();
#elif CONFIG_NETWORK_SERVICES_SCENARIO_OTA
    run_net_ota();
#elif CONFIG_NETWORK_SERVICES_SCENARIO_WIFI_PROV
    run_net_wifi_prov();
#elif CONFIG_NETWORK_SERVICES_SCENARIO_LOCAL_CTRL
    run_net_local_ctrl();
#endif
}
