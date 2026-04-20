/**
 * @file target.h
 * @brief Target identification and per-feature capability query.
 *
 * Use ::blusys_target_supports to guard calls to modules that are not available
 * on every supported chip (e.g. MCPWM, PCNT, touch). All functions are
 * stateless and safe to call from any task.
 */

#ifndef BLUSYS_TARGET_H
#define BLUSYS_TARGET_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Chip family currently running the firmware. */
typedef enum {
    BLUSYS_TARGET_UNKNOWN = 0,  /**< Unrecognized or unsupported target. */
    BLUSYS_TARGET_ESP32,        /**< Original ESP32. */
    BLUSYS_TARGET_ESP32C3,      /**< ESP32-C3. */
    BLUSYS_TARGET_ESP32S3,      /**< ESP32-S3. */
} blusys_target_t;

/**
 * @brief Blusys feature identifiers used by ::blusys_target_supports.
 *
 * One entry per HAL / driver / service module whose availability varies
 * between targets. ::BLUSYS_FEATURE_COUNT is a sentinel, not a valid feature.
 */
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
    BLUSYS_FEATURE_FS,
    BLUSYS_FEATURE_ESPNOW,
    BLUSYS_FEATURE_BLE_GATT,
    BLUSYS_FEATURE_BUTTON,
    BLUSYS_FEATURE_LED_STRIP,
    BLUSYS_FEATURE_CONSOLE,
    BLUSYS_FEATURE_FATFS,
    BLUSYS_FEATURE_SD_SPI,
    BLUSYS_FEATURE_POWER_MGMT,
    BLUSYS_FEATURE_WS_CLIENT,
    BLUSYS_FEATURE_WIFI_PROV,
    BLUSYS_FEATURE_LCD,
    BLUSYS_FEATURE_ENCODER,
    BLUSYS_FEATURE_BUZZER,
    BLUSYS_FEATURE_ONE_WIRE,
    BLUSYS_FEATURE_SEVEN_SEG,
    BLUSYS_FEATURE_DHT,
    BLUSYS_FEATURE_USB_HOST,
    BLUSYS_FEATURE_USB_DEVICE,
    BLUSYS_FEATURE_USB_HID,
    BLUSYS_FEATURE_GPIO_EXPANDER,
    BLUSYS_FEATURE_EFUSE,
    BLUSYS_FEATURE_ULP,
    BLUSYS_FEATURE_WIFI_MESH,
    BLUSYS_FEATURE_COUNT,        /**< Number of entries; not a valid feature. */
} blusys_feature_t;

/** @brief Chip family the firmware was built for. */
blusys_target_t blusys_target_get(void);

/** @brief Short human-readable name of the current target (e.g. "esp32s3"). */
const char *blusys_target_name(void);

/** @brief Number of CPU cores on the current target. */
uint8_t blusys_target_cpu_cores(void);

/**
 * @brief Whether @p feature is available on the current target.
 *
 * Use this before calling into modules that return
 * ::BLUSYS_ERR_NOT_SUPPORTED on unsupported targets (MCPWM, PCNT, touch, DAC,
 * temperature sensor, …).
 *
 * @param feature  Feature identifier.
 * @return `true` if the feature is compiled in and supported, `false` otherwise.
 */
bool blusys_target_supports(blusys_feature_t feature);

#ifdef __cplusplus
}
#endif

#endif
