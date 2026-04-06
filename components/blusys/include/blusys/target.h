#ifndef BLUSYS_TARGET_H
#define BLUSYS_TARGET_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    BLUSYS_TARGET_UNKNOWN = 0,
    BLUSYS_TARGET_ESP32,
    BLUSYS_TARGET_ESP32C3,
    BLUSYS_TARGET_ESP32S3,
} blusys_target_t;

typedef enum {
    BLUSYS_FEATURE_GPIO = 0,
    BLUSYS_FEATURE_UART,
    BLUSYS_FEATURE_I2C,
    BLUSYS_FEATURE_SPI,
    BLUSYS_FEATURE_PWM,
    BLUSYS_FEATURE_ADC,
    BLUSYS_FEATURE_TIMER,
    BLUSYS_FEATURE_PCNT,
    BLUSYS_FEATURE_RMT,
    BLUSYS_FEATURE_TWAI,
    BLUSYS_FEATURE_I2S,
    BLUSYS_FEATURE_TOUCH,
    BLUSYS_FEATURE_DAC,
    BLUSYS_FEATURE_SDMMC,
    BLUSYS_FEATURE_TEMP_SENSOR,
    BLUSYS_FEATURE_WDT,
    BLUSYS_FEATURE_SLEEP,
    BLUSYS_FEATURE_MCPWM,
    BLUSYS_FEATURE_SDM,
    BLUSYS_FEATURE_I2C_SLAVE,
    BLUSYS_FEATURE_SPI_SLAVE,
    BLUSYS_FEATURE_I2S_RX,
    BLUSYS_FEATURE_RMT_RX,
    BLUSYS_FEATURE_WIFI,
    BLUSYS_FEATURE_NVS,
    BLUSYS_FEATURE_HTTP_CLIENT,
    BLUSYS_FEATURE_MQTT,
    BLUSYS_FEATURE_HTTP_SERVER,
    BLUSYS_FEATURE_OTA,
    BLUSYS_FEATURE_SNTP,
    BLUSYS_FEATURE_MDNS,
    BLUSYS_FEATURE_BLUETOOTH,
    BLUSYS_FEATURE_COUNT,
} blusys_feature_t;

blusys_target_t blusys_target_get(void);
const char *blusys_target_name(void);
uint8_t blusys_target_cpu_cores(void);
bool blusys_target_supports(blusys_feature_t feature);

#ifdef __cplusplus
}
#endif

#endif
