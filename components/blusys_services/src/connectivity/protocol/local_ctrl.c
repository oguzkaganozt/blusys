#include "blusys/connectivity/protocol/local_ctrl.h"

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "soc/soc_caps.h"

#if SOC_WIFI_SUPPORTED

#include "blusys/connectivity/protocol/http_server.h"

#define BLUSYS_LOCAL_CTRL_DEFAULT_BODY_LEN      256
#define BLUSYS_LOCAL_CTRL_DEFAULT_RESPONSE_LEN  512
#define BLUSYS_LOCAL_CTRL_ACTION_PREFIX         "/api/actions/"
#define APPEND_LIT(buf, buf_len, used, lit) append_bytes((buf), (buf_len), (used), (lit), sizeof(lit) - 1)

typedef struct {
    char *name;
    char *label;
    char *uri;
    blusys_local_ctrl_action_cb_t handler;
} blusys_local_ctrl_owned_action_t;

typedef enum {
    BLUSYS_LOCAL_CTRL_ROUTE_ROOT = 0,
    BLUSYS_LOCAL_CTRL_ROUTE_INFO,
    BLUSYS_LOCAL_CTRL_ROUTE_STATUS,
    BLUSYS_LOCAL_CTRL_ROUTE_ACTION,
} blusys_local_ctrl_route_kind_t;

typedef struct {
    struct blusys_local_ctrl *ctrl;
    blusys_local_ctrl_route_kind_t kind;
    size_t action_index;
} blusys_local_ctrl_route_ctx_t;

struct blusys_local_ctrl {
    blusys_http_server_t *server;
    volatile bool running;
    char *device_name;
    uint16_t http_port;
    size_t max_body_len;
    size_t max_response_len;
    blusys_local_ctrl_status_cb_t status_cb;
    void *user_ctx;
    blusys_local_ctrl_owned_action_t *actions;
    size_t action_count;
    blusys_http_server_route_t *routes;
    blusys_local_ctrl_route_ctx_t *route_ctxs;
    size_t route_count;
};

static char *dup_str(const char *s)
{
    if (s == NULL) {
        return NULL;
    }

    size_t len = strlen(s);
    char *copy = malloc(len + 1);
    if (copy == NULL) {
        return NULL;
    }

    memcpy(copy, s, len + 1);
    return copy;
}

static bool append_bytes(char *buf, size_t buf_len, size_t *used, const char *src, size_t src_len)
{
    if ((*used + src_len + 1) > buf_len) {
        return false;
    }

    memcpy(buf + *used, src, src_len);
    *used += src_len;
    buf[*used] = '\0';
    return true;
}

static bool append_char(char *buf, size_t buf_len, size_t *used, char c)
{
    return append_bytes(buf, buf_len, used, &c, 1);
}

static bool appendf(char *buf, size_t buf_len, size_t *used, const char *fmt, ...)
{
    if (*used >= buf_len) {
        return false;
    }

    va_list ap;
    va_start(ap, fmt);
    int written = vsnprintf(buf + *used, buf_len - *used, fmt, ap);
    va_end(ap);
    if ((written < 0) || ((size_t)written >= (buf_len - *used))) {
        return false;
    }

    *used += (size_t)written;
    return true;
}

static bool append_json_escaped(char *buf, size_t buf_len, size_t *used, const char *src)
{
    if (src == NULL) {
        return APPEND_LIT(buf, buf_len, used, "null");
    }

    for (const unsigned char *p = (const unsigned char *)src; *p != '\0'; ++p) {
        switch (*p) {
            case '"':
                if (!APPEND_LIT(buf, buf_len, used, "\\\"")) {
                    return false;
                }
                break;
            case '\\':
                if (!APPEND_LIT(buf, buf_len, used, "\\\\")) {
                    return false;
                }
                break;
            case '\n':
                if (!APPEND_LIT(buf, buf_len, used, "\\n")) {
                    return false;
                }
                break;
            case '\r':
                if (!APPEND_LIT(buf, buf_len, used, "\\r")) {
                    return false;
                }
                break;
            case '\t':
                if (!APPEND_LIT(buf, buf_len, used, "\\t")) {
                    return false;
                }
                break;
            default:
                if (*p < 0x20U) {
                    if (!appendf(buf, buf_len, used, "\\u%04x", (unsigned)*p)) {
                        return false;
                    }
                } else if (!append_char(buf, buf_len, used, (char)*p)) {
                    return false;
                }
                break;
        }
    }

    return true;
}

static bool append_html_escaped(char *buf, size_t buf_len, size_t *used, const char *src)
{
    if (src == NULL) {
        return true;
    }

    for (const unsigned char *p = (const unsigned char *)src; *p != '\0'; ++p) {
        switch (*p) {
            case '&':
                if (!APPEND_LIT(buf, buf_len, used, "&amp;")) {
                    return false;
                }
                break;
            case '<':
                if (!APPEND_LIT(buf, buf_len, used, "&lt;")) {
                    return false;
                }
                break;
            case '>':
                if (!APPEND_LIT(buf, buf_len, used, "&gt;")) {
                    return false;
                }
                break;
            case '"':
                if (!APPEND_LIT(buf, buf_len, used, "&quot;")) {
                    return false;
                }
                break;
            case '\'':
                if (!APPEND_LIT(buf, buf_len, used, "&#39;")) {
                    return false;
                }
                break;
            default:
                if (!append_char(buf, buf_len, used, (char)*p)) {
                    return false;
                }
                break;
        }
    }

    return true;
}

static bool valid_action_name(const char *name)
{
    if ((name == NULL) || (*name == '\0')) {
        return false;
    }

    for (const unsigned char *p = (const unsigned char *)name; *p != '\0'; ++p) {
        if (!(isalnum(*p) || (*p == '-') || (*p == '_'))) {
            return false;
        }
    }

    return true;
}

static void free_owned_action(blusys_local_ctrl_owned_action_t *action)
{
    if (action == NULL) {
        return;
    }

    free(action->name);
    free(action->label);
    free(action->uri);
}

static void destroy_ctrl(blusys_local_ctrl_t *ctrl)
{
    if (ctrl == NULL) {
        return;
    }

    if (ctrl->server != NULL) {
        (void)blusys_http_server_close(ctrl->server);
    }

    for (size_t i = 0; i < ctrl->action_count; i++) {
        free_owned_action(&ctrl->actions[i]);
    }

    free(ctrl->actions);
    free(ctrl->routes);
    free(ctrl->route_ctxs);
    free(ctrl->device_name);
    free(ctrl);
}

static blusys_err_t send_json_literal(blusys_http_server_req_t *req, int status_code, const char *body)
{
    return blusys_http_server_resp_send(req, status_code, "application/json", body, -1);
}

static blusys_err_t send_plain_error(blusys_http_server_req_t *req, int status_code, const char *body)
{
    return blusys_http_server_resp_send(req, status_code, "text/plain", body, -1);
}

static blusys_err_t read_request_body(blusys_local_ctrl_t *ctrl,
                                      blusys_http_server_req_t *req,
                                      uint8_t **out_body,
                                      size_t *out_len)
{
    *out_body = NULL;
    *out_len = 0;

    size_t content_len = blusys_http_server_req_content_len(req);
    if (content_len == 0) {
        return BLUSYS_OK;
    }
    if (content_len > ctrl->max_body_len) {
        uint8_t discard_buf[128];
        size_t remaining = content_len;

        while (remaining > 0) {
            size_t chunk = (remaining < sizeof(discard_buf)) ? remaining : sizeof(discard_buf);
            size_t received = 0;
            blusys_err_t err = blusys_http_server_req_recv(req, discard_buf, chunk, &received);
            if (err != BLUSYS_OK) {
                return err;
            }
            if (received == 0) {
                return BLUSYS_ERR_IO;
            }
            remaining -= received;
        }

        return BLUSYS_ERR_INVALID_ARG;
    }

    uint8_t *body = malloc(content_len + 1);
    if (body == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    size_t total = 0;
    while (total < content_len) {
        size_t received = 0;
        blusys_err_t err = blusys_http_server_req_recv(req, body + total, content_len - total, &received);
        if (err != BLUSYS_OK) {
            free(body);
            return err;
        }
        if (received == 0) {
            free(body);
            return BLUSYS_ERR_IO;
        }
        total += received;
    }

    body[total] = '\0';
    *out_body = body;
    *out_len = total;
    return BLUSYS_OK;
}

static char *build_info_json(const blusys_local_ctrl_t *ctrl)
{
    size_t buf_len = 256 + (ctrl->action_count * 96);
    buf_len += strlen(ctrl->device_name) * 6;
    for (size_t i = 0; i < ctrl->action_count; i++) {
        buf_len += strlen(ctrl->actions[i].name) * 6;
        buf_len += strlen(ctrl->actions[i].label) * 6;
    }

    char *buf = calloc(1, buf_len);
    if (buf == NULL) {
        return NULL;
    }

    size_t used = 0;
    if (!APPEND_LIT(buf, buf_len, &used, "{\"device_name\":\"") ||
        !append_json_escaped(buf, buf_len, &used, ctrl->device_name) ||
        !appendf(buf, buf_len, &used,
                 "\",\"http_port\":%u,\"info_path\":\"/api/info\",\"status_path\":",
                 (unsigned)ctrl->http_port) ||
        ((ctrl->status_cb != NULL)
            ? (!APPEND_LIT(buf, buf_len, &used, "\"/api/status\""))
            : (!APPEND_LIT(buf, buf_len, &used, "null"))) ||
        !appendf(buf, buf_len, &used, ",\"action_count\":%u,\"actions\":[",
                 (unsigned)ctrl->action_count)) {
        free(buf);
        return NULL;
    }

    for (size_t i = 0; i < ctrl->action_count; i++) {
        if ((i > 0) && !append_char(buf, buf_len, &used, ',')) {
            free(buf);
            return NULL;
        }
        if (!APPEND_LIT(buf, buf_len, &used, "{\"name\":\"") ||
            !append_json_escaped(buf, buf_len, &used, ctrl->actions[i].name) ||
            !APPEND_LIT(buf, buf_len, &used, "\",\"label\":\"") ||
            !append_json_escaped(buf, buf_len, &used, ctrl->actions[i].label) ||
            !APPEND_LIT(buf, buf_len, &used, "\"}")) {
            free(buf);
            return NULL;
        }
    }

    if (!APPEND_LIT(buf, buf_len, &used, "]}")) {
        free(buf);
        return NULL;
    }

    return buf;
}

static char *build_root_page(const blusys_local_ctrl_t *ctrl)
{
    size_t buf_len = 3072 + (ctrl->action_count * 224);
    buf_len += strlen(ctrl->device_name) * 8;
    for (size_t i = 0; i < ctrl->action_count; i++) {
        buf_len += strlen(ctrl->actions[i].name) * 8;
        buf_len += strlen(ctrl->actions[i].label) * 8;
    }

    char *buf = calloc(1, buf_len);
    if (buf == NULL) {
        return NULL;
    }

    size_t used = 0;
    if (!append_bytes(buf, buf_len, &used,
                      "<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>",
                      sizeof("<!doctype html><html><head><meta charset=\"utf-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1\"><title>") - 1) ||
        !append_html_escaped(buf, buf_len, &used, ctrl->device_name) ||
        !append_bytes(buf, buf_len, &used,
                      "</title><style>body{margin:0;font-family:Inter,Arial,sans-serif;background:#0f172a;color:#e2e8f0;}main{max-width:860px;margin:0 auto;padding:32px 20px 56px;}h1{margin:0 0 8px;font-size:2rem;}p{color:#94a3b8;line-height:1.5;}section{background:#111827;border:1px solid #334155;border-radius:18px;padding:20px;margin-top:18px;box-shadow:0 12px 32px rgba(15,23,42,.24);}button{border:0;border-radius:12px;padding:12px 16px;background:linear-gradient(135deg,#38bdf8,#2563eb);color:#fff;font-weight:600;cursor:pointer;}button:hover{filter:brightness(1.08);}button:active{transform:translateY(1px);}pre{margin:0;white-space:pre-wrap;word-break:break-word;font-size:.95rem;color:#bfdbfe;}code{font-size:.92rem;color:#93c5fd;}#actions{display:flex;flex-wrap:wrap;gap:12px;}#message{min-height:1.2em;color:#f8fafc;}a{color:#7dd3fc;text-decoration:none;}a:hover{text-decoration:underline;}</style></head><body><main><p>Blusys local control</p><h1>",
                      sizeof("</title><style>body{margin:0;font-family:Inter,Arial,sans-serif;background:#0f172a;color:#e2e8f0;}main{max-width:860px;margin:0 auto;padding:32px 20px 56px;}h1{margin:0 0 8px;font-size:2rem;}p{color:#94a3b8;line-height:1.5;}section{background:#111827;border:1px solid #334155;border-radius:18px;padding:20px;margin-top:18px;box-shadow:0 12px 32px rgba(15,23,42,.24);}button{border:0;border-radius:12px;padding:12px 16px;background:linear-gradient(135deg,#38bdf8,#2563eb);color:#fff;font-weight:600;cursor:pointer;}button:hover{filter:brightness(1.08);}button:active{transform:translateY(1px);}pre{margin:0;white-space:pre-wrap;word-break:break-word;font-size:.95rem;color:#bfdbfe;}code{font-size:.92rem;color:#93c5fd;}#actions{display:flex;flex-wrap:wrap;gap:12px;}#message{min-height:1.2em;color:#f8fafc;}a{color:#7dd3fc;text-decoration:none;}a:hover{text-decoration:underline;}</style></head><body><main><p>Blusys local control</p><h1>") - 1) ||
        !append_html_escaped(buf, buf_len, &used, ctrl->device_name) ||
        !append_bytes(buf, buf_len, &used,
                      "</h1><p>Use the built-in page for quick browser control, or call the JSON endpoints directly from your own app.</p><section><h2>Actions</h2><div id=\"actions\">",
                      sizeof("</h1><p>Use the built-in page for quick browser control, or call the JSON endpoints directly from your own app.</p><section><h2>Actions</h2><div id=\"actions\">") - 1)) {
        free(buf);
        return NULL;
    }

    if (ctrl->action_count == 0) {
        if (!APPEND_LIT(buf, buf_len, &used, "<p>No actions registered.</p>")) {
            free(buf);
            return NULL;
        }
    } else {
        for (size_t i = 0; i < ctrl->action_count; i++) {
            if (!APPEND_LIT(buf, buf_len, &used, "<button type=\"button\" data-action=\"") ||
                !append_html_escaped(buf, buf_len, &used, ctrl->actions[i].name) ||
                !APPEND_LIT(buf, buf_len, &used, "\">") ||
                !append_html_escaped(buf, buf_len, &used, ctrl->actions[i].label) ||
                !APPEND_LIT(buf, buf_len, &used, "</button>")) {
                free(buf);
                return NULL;
            }
        }
    }

    if (!append_bytes(buf, buf_len, &used,
                      "</div><p id=\"message\"></p></section><section><h2>Status</h2>",
                      sizeof("</div><p id=\"message\"></p></section><section><h2>Status</h2>") - 1)) {
        free(buf);
        return NULL;
    }

    if (ctrl->status_cb != NULL) {
        if (!APPEND_LIT(buf, buf_len, &used,
                        "<pre id=\"status\">Loading...</pre>")) {
            free(buf);
            return NULL;
        }
    } else if (!APPEND_LIT(buf, buf_len, &used,
                           "<p>Status callback not configured.</p>")) {
        free(buf);
        return NULL;
    }

    if (!append_bytes(buf, buf_len, &used,
                      "</section><section><h2>Endpoints</h2><p><a href=\"/api/info\"><code>/api/info</code></a></p>",
                      sizeof("</section><section><h2>Endpoints</h2><p><a href=\"/api/info\"><code>/api/info</code></a></p>") - 1)) {
        free(buf);
        return NULL;
    }

    if (ctrl->status_cb != NULL) {
        if (!APPEND_LIT(buf, buf_len, &used,
                        "<p><a href=\"/api/status\"><code>/api/status</code></a></p>")) {
            free(buf);
            return NULL;
        }
    }

    if (!append_bytes(buf, buf_len, &used,
                      "</section><script>const messageEl=document.getElementById('message');async function runAction(name){messageEl.textContent='Running '+name+'...';try{const res=await fetch('/api/actions/'+name,{method:'POST'});const text=await res.text();messageEl.textContent=(res.ok?'Done: ':'Error: ')+text;if(window.refreshStatus){window.refreshStatus();}}catch(err){messageEl.textContent='Request failed: '+err;}}document.querySelectorAll('[data-action]').forEach((button)=>{button.addEventListener('click',()=>runAction(button.dataset.action));});",
                      sizeof("</section><script>const messageEl=document.getElementById('message');async function runAction(name){messageEl.textContent='Running '+name+'...';try{const res=await fetch('/api/actions/'+name,{method:'POST'});const text=await res.text();messageEl.textContent=(res.ok?'Done: ':'Error: ')+text;if(window.refreshStatus){window.refreshStatus();}}catch(err){messageEl.textContent='Request failed: '+err;}}document.querySelectorAll('[data-action]').forEach((button)=>{button.addEventListener('click',()=>runAction(button.dataset.action));});") - 1)) {
        free(buf);
        return NULL;
    }

    if (ctrl->status_cb != NULL) {
        if (!append_bytes(buf, buf_len, &used,
                          "window.refreshStatus=async function(){const statusEl=document.getElementById('status');try{const res=await fetch('/api/status',{cache:'no-store'});const text=await res.text();if(!res.ok){statusEl.textContent='Status request failed ('+res.status+')';return;}try{statusEl.textContent=JSON.stringify(JSON.parse(text),null,2);}catch(_unused){statusEl.textContent=text;}}catch(err){statusEl.textContent='Status request failed: '+err;}};window.refreshStatus();setInterval(window.refreshStatus,2000);",
                          sizeof("window.refreshStatus=async function(){const statusEl=document.getElementById('status');try{const res=await fetch('/api/status',{cache:'no-store'});const text=await res.text();if(!res.ok){statusEl.textContent='Status request failed ('+res.status+')';return;}try{statusEl.textContent=JSON.stringify(JSON.parse(text),null,2);}catch(_unused){statusEl.textContent=text;}}catch(err){statusEl.textContent='Status request failed: '+err;}};window.refreshStatus();setInterval(window.refreshStatus,2000);") - 1)) {
            free(buf);
            return NULL;
        }
    }

    if (!APPEND_LIT(buf, buf_len, &used, "</script></main></body></html>")) {
        free(buf);
        return NULL;
    }

    return buf;
}

static blusys_err_t handle_root(blusys_http_server_req_t *req, void *user_ctx)
{
    blusys_local_ctrl_route_ctx_t *ctx = user_ctx;
    char *page = build_root_page(ctx->ctrl);
    if (page == NULL) {
        return send_plain_error(req, 500, "local_ctrl: failed to build page");
    }

    blusys_err_t err = blusys_http_server_resp_send(req, 200, "text/html", page, -1);
    free(page);
    return err;
}

static blusys_err_t handle_info(blusys_http_server_req_t *req, void *user_ctx)
{
    blusys_local_ctrl_route_ctx_t *ctx = user_ctx;
    char *json = build_info_json(ctx->ctrl);
    if (json == NULL) {
        return send_json_literal(req, 500, "{\"ok\":false,\"error\":\"no_mem\"}");
    }

    blusys_err_t err = blusys_http_server_resp_send(req, 200, "application/json", json, -1);
    free(json);
    return err;
}

static blusys_err_t handle_status(blusys_http_server_req_t *req, void *user_ctx)
{
    blusys_local_ctrl_route_ctx_t *ctx = user_ctx;
    blusys_local_ctrl_t *ctrl = ctx->ctrl;
    if (ctrl->status_cb == NULL) {
        return send_json_literal(req, 404, "{\"ok\":false,\"error\":\"status_unavailable\"}");
    }

    char *json = calloc(1, ctrl->max_response_len + 1);
    if (json == NULL) {
        return send_json_literal(req, 500, "{\"ok\":false,\"error\":\"no_mem\"}");
    }

    size_t out_len = 0;
    blusys_err_t cb_err = ctrl->status_cb(json, ctrl->max_response_len, &out_len, ctrl->user_ctx);
    if ((cb_err != BLUSYS_OK) || (out_len > ctrl->max_response_len)) {
        free(json);
        return send_json_literal(req, 500, "{\"ok\":false,\"error\":\"status_failed\"}");
    }

    if (out_len == 0) {
        free(json);
        return send_json_literal(req, 200, "{}");
    }

    blusys_err_t err = blusys_http_server_resp_send(req, 200, "application/json", json, (int)out_len);
    free(json);
    return err;
}

static blusys_err_t handle_action(blusys_http_server_req_t *req, void *user_ctx)
{
    blusys_local_ctrl_route_ctx_t *ctx = user_ctx;
    blusys_local_ctrl_t *ctrl = ctx->ctrl;
    if (ctx->action_index >= ctrl->action_count) {
        return send_json_literal(req, 404, "{\"ok\":false,\"error\":\"action_not_found\"}");
    }

    blusys_local_ctrl_owned_action_t *action = &ctrl->actions[ctx->action_index];
    uint8_t *body = NULL;
    size_t body_len = 0;
    blusys_err_t err = read_request_body(ctrl, req, &body, &body_len);
    if (err == BLUSYS_ERR_INVALID_ARG) {
        return send_json_literal(req, 400, "{\"ok\":false,\"error\":\"body_too_large\"}");
    }
    if (err == BLUSYS_ERR_NO_MEM) {
        return send_json_literal(req, 500, "{\"ok\":false,\"error\":\"no_mem\"}");
    }
    if (err != BLUSYS_OK) {
        return send_json_literal(req, 400, "{\"ok\":false,\"error\":\"read_failed\"}");
    }

    char *json = calloc(1, ctrl->max_response_len + 1);
    if (json == NULL) {
        free(body);
        return send_json_literal(req, 500, "{\"ok\":false,\"error\":\"no_mem\"}");
    }

    size_t out_len = 0;
    int status_code = 200;
    blusys_err_t cb_err = action->handler(action->name,
                                          body,
                                          body_len,
                                          json,
                                          ctrl->max_response_len,
                                          &out_len,
                                          &status_code,
                                          ctrl->user_ctx);
    free(body);
    if ((cb_err != BLUSYS_OK) || (out_len > ctrl->max_response_len)) {
        free(json);
        return send_json_literal(req, 500, "{\"ok\":false,\"error\":\"action_failed\"}");
    }

    if (status_code == 0) {
        status_code = 200;
    }

    if (out_len == 0) {
        free(json);
        return send_json_literal(req, status_code, "{\"ok\":true}");
    }

    err = blusys_http_server_resp_send(req, status_code, "application/json", json, (int)out_len);
    free(json);
    return err;
}

static blusys_err_t copy_actions(const blusys_local_ctrl_config_t *config, blusys_local_ctrl_t *ctrl)
{
    if (config->action_count == 0) {
        return BLUSYS_OK;
    }

    ctrl->actions = calloc(config->action_count, sizeof(*ctrl->actions));
    if (ctrl->actions == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    ctrl->action_count = config->action_count;

    for (size_t i = 0; i < config->action_count; i++) {
        const blusys_local_ctrl_action_t *src = &config->actions[i];
        blusys_local_ctrl_owned_action_t *dst = &ctrl->actions[i];
        if (!valid_action_name(src->name) || (src->handler == NULL)) {
            return BLUSYS_ERR_INVALID_ARG;
        }
        for (size_t j = 0; j < i; j++) {
            if (strcmp(ctrl->actions[j].name, src->name) == 0) {
                return BLUSYS_ERR_INVALID_ARG;
            }
        }

        dst->name = dup_str(src->name);
        dst->label = dup_str((src->label != NULL) ? src->label : src->name);
        dst->uri = malloc(strlen(BLUSYS_LOCAL_CTRL_ACTION_PREFIX) + strlen(src->name) + 1);
        if ((dst->name == NULL) || (dst->label == NULL) || (dst->uri == NULL)) {
            return BLUSYS_ERR_NO_MEM;
        }

        strcpy(dst->uri, BLUSYS_LOCAL_CTRL_ACTION_PREFIX);
        strcat(dst->uri, src->name);
        dst->handler = src->handler;
    }
    return BLUSYS_OK;
}

static blusys_err_t build_routes(blusys_local_ctrl_t *ctrl)
{
    ctrl->route_count = 2 + ctrl->action_count + ((ctrl->status_cb != NULL) ? 1 : 0);
    ctrl->routes = calloc(ctrl->route_count, sizeof(*ctrl->routes));
    ctrl->route_ctxs = calloc(ctrl->route_count, sizeof(*ctrl->route_ctxs));
    if ((ctrl->routes == NULL) || (ctrl->route_ctxs == NULL)) {
        return BLUSYS_ERR_NO_MEM;
    }

    size_t idx = 0;

    ctrl->route_ctxs[idx] = (blusys_local_ctrl_route_ctx_t) {
        .ctrl = ctrl,
        .kind = BLUSYS_LOCAL_CTRL_ROUTE_ROOT,
    };
    ctrl->routes[idx] = (blusys_http_server_route_t) {
        .uri = "/",
        .method = BLUSYS_HTTP_METHOD_GET,
        .handler = handle_root,
        .user_ctx = &ctrl->route_ctxs[idx],
    };
    idx++;

    ctrl->route_ctxs[idx] = (blusys_local_ctrl_route_ctx_t) {
        .ctrl = ctrl,
        .kind = BLUSYS_LOCAL_CTRL_ROUTE_INFO,
    };
    ctrl->routes[idx] = (blusys_http_server_route_t) {
        .uri = "/api/info",
        .method = BLUSYS_HTTP_METHOD_GET,
        .handler = handle_info,
        .user_ctx = &ctrl->route_ctxs[idx],
    };
    idx++;

    if (ctrl->status_cb != NULL) {
        ctrl->route_ctxs[idx] = (blusys_local_ctrl_route_ctx_t) {
            .ctrl = ctrl,
            .kind = BLUSYS_LOCAL_CTRL_ROUTE_STATUS,
        };
        ctrl->routes[idx] = (blusys_http_server_route_t) {
            .uri = "/api/status",
            .method = BLUSYS_HTTP_METHOD_GET,
            .handler = handle_status,
            .user_ctx = &ctrl->route_ctxs[idx],
        };
        idx++;
    }

    for (size_t i = 0; i < ctrl->action_count; i++, idx++) {
        ctrl->route_ctxs[idx] = (blusys_local_ctrl_route_ctx_t) {
            .ctrl = ctrl,
            .kind = BLUSYS_LOCAL_CTRL_ROUTE_ACTION,
            .action_index = i,
        };
        ctrl->routes[idx] = (blusys_http_server_route_t) {
            .uri = ctrl->actions[i].uri,
            .method = BLUSYS_HTTP_METHOD_POST,
            .handler = handle_action,
            .user_ctx = &ctrl->route_ctxs[idx],
        };
    }

    return BLUSYS_OK;
}

blusys_err_t blusys_local_ctrl_open(const blusys_local_ctrl_config_t *config,
                                    blusys_local_ctrl_t **out_ctrl)
{
    if ((config == NULL) || (out_ctrl == NULL) || (config->device_name == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }
    if ((config->action_count > 0) && (config->actions == NULL)) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    blusys_local_ctrl_t *ctrl = calloc(1, sizeof(*ctrl));
    if (ctrl == NULL) {
        return BLUSYS_ERR_NO_MEM;
    }

    ctrl->device_name = dup_str(config->device_name);
    if (ctrl->device_name == NULL) {
        destroy_ctrl(ctrl);
        return BLUSYS_ERR_NO_MEM;
    }

    ctrl->http_port = (config->http_port != 0) ? config->http_port : 80;
    ctrl->max_body_len = (config->max_body_len != 0) ? config->max_body_len : BLUSYS_LOCAL_CTRL_DEFAULT_BODY_LEN;
    ctrl->max_response_len = (config->max_response_len != 0)
                           ? config->max_response_len
                           : BLUSYS_LOCAL_CTRL_DEFAULT_RESPONSE_LEN;
    ctrl->status_cb = config->status_cb;
    ctrl->user_ctx = config->user_ctx;

    blusys_err_t err = copy_actions(config, ctrl);
    if (err != BLUSYS_OK) {
        destroy_ctrl(ctrl);
        return err;
    }

    err = build_routes(ctrl);
    if (err != BLUSYS_OK) {
        destroy_ctrl(ctrl);
        return err;
    }

    blusys_http_server_config_t server_cfg = {
        .port = ctrl->http_port,
        .routes = ctrl->routes,
        .route_count = ctrl->route_count,
    };
    err = blusys_http_server_open(&server_cfg, &ctrl->server);
    if (err != BLUSYS_OK) {
        destroy_ctrl(ctrl);
        return err;
    }

    ctrl->running = true;
    *out_ctrl = ctrl;
    return BLUSYS_OK;
}

blusys_err_t blusys_local_ctrl_close(blusys_local_ctrl_t *ctrl)
{
    if (ctrl == NULL) {
        return BLUSYS_ERR_INVALID_ARG;
    }

    ctrl->running = false;
    destroy_ctrl(ctrl);
    return BLUSYS_OK;
}

bool blusys_local_ctrl_is_running(blusys_local_ctrl_t *ctrl)
{
    if ((ctrl == NULL) || (ctrl->server == NULL)) {
        return false;
    }

    return ctrl->running && blusys_http_server_is_running(ctrl->server);
}

#else /* !SOC_WIFI_SUPPORTED */

blusys_err_t blusys_local_ctrl_open(const blusys_local_ctrl_config_t *config,
                                    blusys_local_ctrl_t **out_ctrl)
{
    (void)config;
    (void)out_ctrl;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

blusys_err_t blusys_local_ctrl_close(blusys_local_ctrl_t *ctrl)
{
    (void)ctrl;
    return BLUSYS_ERR_NOT_SUPPORTED;
}

bool blusys_local_ctrl_is_running(blusys_local_ctrl_t *ctrl)
{
    (void)ctrl;
    return false;
}

#endif /* SOC_WIFI_SUPPORTED */
