#include "blusys/observe/error_domain.h"

const char *blusys_err_domain_string(blusys_err_domain_t domain)
{
    switch (domain) {
    case err_domain_generic:            return "generic";

    case err_domain_framework_observe:  return "framework.observe";

    case err_domain_driver_display:     return "driver.display";
    case err_domain_driver_dht:         return "driver.dht";
    case err_domain_driver_encoder:     return "driver.encoder";
    case err_domain_driver_seven_seg:   return "driver.seven_seg";
    case err_domain_driver_lcd:         return "driver.lcd";
    case err_domain_driver_button:      return "driver.button";

    case err_domain_storage_fs:         return "storage.fs";
    case err_domain_storage_fatfs:      return "storage.fatfs";

    case err_domain_hal_gpio:           return "hal.gpio";
    case err_domain_hal_timer:          return "hal.timer";
    case err_domain_hal_uart:           return "hal.uart";

    case err_domain_persistence:        return "persistence";

    case err_domain_count:              break;
    }
    return "?";
}
