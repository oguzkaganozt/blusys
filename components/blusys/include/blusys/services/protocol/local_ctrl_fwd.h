#ifndef BLUSYS_LOCAL_CTRL_FWD_H
#define BLUSYS_LOCAL_CTRL_FWD_H

#include <stddef.h>
#include <stdint.h>

#include "blusys/hal/error.h"

#ifdef __cplusplus
extern "C" {
#endif

struct blusys_local_ctrl;
typedef struct blusys_local_ctrl blusys_local_ctrl_t;

struct blusys_local_ctrl_action;
typedef struct blusys_local_ctrl_action blusys_local_ctrl_action_t;

struct blusys_local_ctrl_config;
typedef struct blusys_local_ctrl_config blusys_local_ctrl_config_t;

typedef blusys_err_t (*blusys_local_ctrl_status_cb_t)(char *json_buf,
                                                      size_t buf_len,
                                                      size_t *out_len,
                                                      void *user_ctx);

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
