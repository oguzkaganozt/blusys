/**
 * @file log.h
 * @brief Legacy tag-based log macros (ESP-IDF `ESP_LOG*` aliases).
 *
 * Thin wrappers over `ESP_LOG{E,W,I,D}`. Retained for subsystems that have not
 * migrated to the structured front-end in blusys/observe/log.h. New code should
 * prefer `BLUSYS_LOG(level, domain, fmt, ...)` from the observe header.
 */

#ifndef BLUSYS_LOG_H
#define BLUSYS_LOG_H

#include "esp_log.h"

/** @brief Log an error line with an ESP-IDF tag. */
#define BLUSYS_LOGE(tag, fmt, ...) ESP_LOGE(tag, fmt, ##__VA_ARGS__)
/** @brief Log a warning line with an ESP-IDF tag. */
#define BLUSYS_LOGW(tag, fmt, ...) ESP_LOGW(tag, fmt, ##__VA_ARGS__)
/** @brief Log an info line with an ESP-IDF tag. */
#define BLUSYS_LOGI(tag, fmt, ...) ESP_LOGI(tag, fmt, ##__VA_ARGS__)
/** @brief Log a debug line with an ESP-IDF tag. */
#define BLUSYS_LOGD(tag, fmt, ...) ESP_LOGD(tag, fmt, ##__VA_ARGS__)

#endif
