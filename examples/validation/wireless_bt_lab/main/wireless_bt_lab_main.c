#include "sdkconfig.h"

void run_bluetooth_basic(void);
void run_ble_gatt_basic(void);

void app_main(void)
{
#if CONFIG_WIRELESS_BT_LAB_SCENARIO_BLUETOOTH_BASIC
    run_bluetooth_basic();
#elif CONFIG_WIRELESS_BT_LAB_SCENARIO_BLE_GATT_BASIC
    run_ble_gatt_basic();
#endif
}
