/*
 * scripts/host/include_host/esp_log.h — host shim for ESP-IDF's log.h.
 *
 * `blusys/log.h` is a thin wrapper around ESP-IDF's `esp_log.h`, so any
 * framework source that logs (e.g. runtime.cpp, encoder.cpp, all five
 * V1 widgets) transitively pulls in `esp_log.h`. On a real ESP32 build
 * the real header comes from the `log` component; on host builds we
 * satisfy the same include path with this minimal, printf-backed shim.
 *
 * This shim is intentionally small:
 *   - It only implements the four log levels blusys/log.h uses
 *     (ESP_LOGE / ESP_LOGW / ESP_LOGI / ESP_LOGD).
 *   - It writes ERROR/WARN to stderr and INFO/DEBUG to stdout, each
 *     prefixed with a level letter + the tag, matching the shape of
 *     the real IDF console output closely enough for visual grepping.
 *   - It is valid C11 and C++20 so the framework core (C++) and
 *     blusys/error.c (C) can both include it without fuss.
 *
 * Do not add anything else to this shim. If a framework source ever
 * needs more from `esp_log.h` than the four BLUSYS_LOG* macros cover,
 * fix blusys/log.h first — the framework's log surface is a narrow,
 * deliberate contract.
 */

#ifndef BLUSYS_HOST_ESP_LOG_H
#define BLUSYS_HOST_ESP_LOG_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

#define ESP_LOGE(tag, fmt, ...) \
    do { fprintf(stderr, "E (%s) " fmt "\n", (tag), ##__VA_ARGS__); } while (0)
#define ESP_LOGW(tag, fmt, ...) \
    do { fprintf(stderr, "W (%s) " fmt "\n", (tag), ##__VA_ARGS__); } while (0)
#define ESP_LOGI(tag, fmt, ...) \
    do { fprintf(stdout, "I (%s) " fmt "\n", (tag), ##__VA_ARGS__); } while (0)
#define ESP_LOGD(tag, fmt, ...) \
    do { fprintf(stdout, "D (%s) " fmt "\n", (tag), ##__VA_ARGS__); } while (0)

#ifdef __cplusplus
}
#endif

#endif  /* BLUSYS_HOST_ESP_LOG_H */
