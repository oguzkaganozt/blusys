/**
 * @file local_ctrl_fwd.h
 * @brief Forward declarations and callback types for `blusys_local_ctrl`.
 *
 * This lets callers declare state-pointer fields or callback signatures
 * without pulling in the full definition of the config struct.
 */

#ifndef BLUSYS_LOCAL_CTRL_FWD_H
#define BLUSYS_LOCAL_CTRL_FWD_H

#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

struct blusys_local_ctrl;
/** @brief Opaque local-control server handle. */
typedef struct blusys_local_ctrl blusys_local_ctrl_t;

struct blusys_local_ctrl_action;
/** @brief One action registered with a local-control server. */
typedef struct blusys_local_ctrl_action blusys_local_ctrl_action_t;

struct blusys_local_ctrl_config;
/** @brief Configuration for ::blusys_local_ctrl_open. */
typedef struct blusys_local_ctrl_config blusys_local_ctrl_config_t;

/**
 * @brief Status callback for the built-in `GET /api/status` route.
 *
 * Serialise the device status as JSON into @p json_buf (up to @p buf_len
 * bytes) and write the number of bytes produced to `*out_len`.
 *
 * @param json_buf  Output buffer.
 * @param buf_len   Capacity of @p json_buf.
 * @param out_len   Output: bytes written (excluding any null terminator).
 * @param user_ctx  User pointer supplied via ::blusys_local_ctrl_config.
 * @return `BLUSYS_OK` on success.
 */
typedef blusys_err_t (*blusys_local_ctrl_status_cb_t)(char *json_buf,
                                                      size_t buf_len,
                                                      size_t *out_len,
                                                      void *user_ctx);

/**
 * @brief Action callback invoked by `POST /api/action/<name>`.
 *
 * Write the JSON response body into @p json_buf and set `*out_status_code`
 * to the HTTP status (default `200`).
 *
 * @param action_name      Name of the triggered action.
 * @param body             Request body (may be NULL for empty).
 * @param body_len         Bytes in @p body.
 * @param json_buf         Output buffer for the JSON response body.
 * @param buf_len          Capacity of @p json_buf.
 * @param out_len          Output: bytes written.
 * @param out_status_code  Output: HTTP status code to return.
 * @param user_ctx         User pointer supplied via ::blusys_local_ctrl_action.
 * @return `BLUSYS_OK` on success.
 */
typedef blusys_err_t (*blusys_local_ctrl_action_cb_t)(const char *action_name,
                                                      const uint8_t *body,
                                                      size_t body_len,
                                                      char *json_buf,
                                                      size_t buf_len,
                                                      size_t *out_len,
                                                      int *out_status_code,
                                                      void *user_ctx);

#ifdef __cplusplus
}
#endif

#endif
