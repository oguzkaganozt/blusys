#ifndef BLUSYS_ESP_ERR_H
#define BLUSYS_ESP_ERR_H

#include "esp_err.h"

#include "blusys/framework/observe/error_domain.h"
#include "blusys/hal/error.h"

/* Map an ESP-IDF error to a generic-domain blusys_err_t. Backward-compatible
 * with all pre-domain callers. */
static inline blusys_err_t blusys_translate_esp_err(esp_err_t err)
{
    switch (err) {
    case ESP_OK:
        return BLUSYS_OK;
    case ESP_ERR_INVALID_ARG:
    case ESP_ERR_INVALID_SIZE:
        return BLUSYS_ERR_INVALID_ARG;
    case ESP_ERR_INVALID_STATE:
        return BLUSYS_ERR_INVALID_STATE;
    case ESP_ERR_NOT_SUPPORTED:
        return BLUSYS_ERR_NOT_SUPPORTED;
    case ESP_ERR_TIMEOUT:
        return BLUSYS_ERR_TIMEOUT;
    case ESP_ERR_NO_MEM:
        return BLUSYS_ERR_NO_MEM;
    case ESP_ERR_NOT_FOUND:
        return BLUSYS_ERR_NOT_FOUND;
    case ESP_ERR_INVALID_RESPONSE:
    case ESP_FAIL:
        return BLUSYS_ERR_IO;
    default:
        return BLUSYS_ERR_INTERNAL;
    }
}

/* Same mapping, but stamp the result with the caller's subsystem domain so
 * structured log and diagnostics can attribute the failure. */
static inline blusys_err_t blusys_translate_esp_err_in(blusys_err_domain_t domain,
                                                       esp_err_t           err)
{
    blusys_err_t generic = blusys_translate_esp_err(err);
    if (generic == BLUSYS_OK) {
        return BLUSYS_OK;
    }
    return BLUSYS_ERR(domain, BLUSYS_ERR_CODE(generic));
}

#endif
