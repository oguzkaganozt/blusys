#pragma once

#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool connected;
    bool connecting;
    bool closing;
    bool worker_active;
} blusys_session_t;

static inline void blusys_session_init(blusys_session_t *session)
{
    if (session != NULL) {
        session->connected = false;
        session->connecting = false;
        session->closing = false;
        session->worker_active = false;
    }
}

static inline bool blusys_session_can_connect(const blusys_session_t *session)
{
    return (session != NULL) && !session->connected && !session->connecting && !session->closing;
}

static inline bool blusys_session_request_connect(blusys_session_t *session)
{
    if (!blusys_session_can_connect(session)) {
        return false;
    }

    session->connecting = true;
    return true;
}

static inline bool blusys_session_finish_connect(blusys_session_t *session, bool connected)
{
    if (session == NULL) {
        return false;
    }

    if (!session->connecting) {
        return connected ? session->connected : false;
    }

    session->connecting = false;
    if (session->closing) {
        session->connected = false;
        return false;
    }

    session->connected = connected;
    return connected;
}

static inline bool blusys_session_request_disconnect(blusys_session_t *session)
{
    if ((session == NULL) || session->closing) {
        return false;
    }

    if (!session->connected && !session->connecting && !session->worker_active) {
        return false;
    }

    session->closing = true;
    return true;
}

static inline void blusys_session_note_disconnected(blusys_session_t *session)
{
    if (session != NULL) {
        session->connected      = false;
        session->connecting     = false;
        session->worker_active  = false;
    }
}

static inline void blusys_session_finish_disconnect(blusys_session_t *session)
{
    if (session != NULL) {
        blusys_session_note_disconnected(session);
        session->closing = false;
    }
}

static inline void blusys_session_set_worker_active(blusys_session_t *session, bool active)
{
    if (session != NULL) {
        session->worker_active = active;
    }
}

static inline bool blusys_session_is_connected(const blusys_session_t *session)
{
    return (session != NULL) && session->connected;
}

static inline bool blusys_session_is_connecting(const blusys_session_t *session)
{
    return (session != NULL) && session->connecting;
}

static inline bool blusys_session_is_closing(const blusys_session_t *session)
{
    return (session != NULL) && session->closing;
}

static inline bool blusys_session_is_worker_active(const blusys_session_t *session)
{
    return (session != NULL) && session->worker_active;
}

#ifdef __cplusplus
}
#endif
