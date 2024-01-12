#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <pthread.h>

#include "curl-websocket.h"

#include "websockets.h"
#include "cog-utils.h"

#define CURLM_LOG(ws, mcode)                                                  \
    logconf_fatal(&ws->conf, "(CURLM code: %d) %s", mcode,                    \
                  curl_multi_strerror(mcode))
#define CASE_RETURN_STR(code)                                                 \
    case code:                                                                \
        return #code

struct websockets {
    /** the logconf structure for logging @see logconf_setup() */
    struct logconf conf;
    /** stores info on the latest transfer performed via websockets */
    struct ws_info info;
    /**
     * the client connections status
     * @note `WS_CONNECTING` set at ws_start()
     *       `WS_CONNECTED` set at ws_on_connect()
     *       `WS_DISCONNECTING` set at start of ws_end() or ws_close()
     *       `WS_DISCONNECTED` set after complete shutdown at ws_end()
     * @see ws_get_status()
     */
    enum ws_status status;
    /** perform non-blocking transfers */
    CURLM *mhandle;
    /** perform/receive individual WebSockets tranfers */
    CURL *ehandle;
    /** timestamp updated every ws_timestamp_update() call */
    uint64_t now_tstamp;
    /** WebSockets connection URL @see ws_set_url() */
    char base_url[512 + 1];
    /** WebSockets connection protocols @see ws_set_url() */
    char protocols[126];
    /** WebSockets callbacks */
    struct ws_callbacks cbs;
    /**
     * capture curl error messages
     * @note should only be accessed after a error code returns
     * @see https://curl.se/libcurl/c/CURLOPT_ERRORBUFFER.html
     */
    char errbuf[CURL_ERROR_SIZE];
    /** lock for functions that may be called in other threads */
    pthread_mutex_t lock;
    /** lock for reading/writing the event-loop timestamp */
    pthread_rwlock_t rwlock;

    /**
     * user-triggered actions
     * @note the user may close the active connection via ws_close()
     */
    enum {
        /** no user action taking place */
        WS_ACTION_NONE = 0,
        /** user called ws_close() */
        WS_ACTION_BEGIN_CLOSE,
        /** succesfully closed connection after ws_close() */
        WS_ACTION_END_CLOSE
    } action;

    /** close context filled at ws_close() */
    struct {
        /** opcode reason for closing */
        enum ws_close_reason code;
        /** textual reason for closing */
        char reason[125 + 1];
    } pending_close;
};

static void _ws_set_status(struct websockets *ws, enum ws_status status);

static int
_ws_curl_tls_check(
    CURL *handle, curl_infotype type, char *data, size_t size, void *userp)
{
    struct websockets *ws = userp;
    (void)data;
    (void)size;

    /* avoid busy-waiting in case loop is kept alive even if the TLS connection
     *        has been closed
     * TODO: look for a better solution */
    if (CURLINFO_TEXT == type && WS_CONNECTED == ws->status
        && strstr(data, "close notify (256)"))
    {
        const char reason[] = "TLS ended connection with a close notify (256)";
        const char *url = NULL, *method = NULL;

        curl_easy_getinfo(handle, CURLINFO_EFFECTIVE_URL, &url);

        logconf_error(&ws->conf, "%s\nLast known URL: %s [@@@_%zu_@@@]",
                      reason, url, ws->info.loginfo.rcounter);

        _ws_set_status(ws, WS_DISCONNECTED);
    }
    return 0;
}

const char *
ws_close_opcode_print(enum ws_close_reason opcode)
{
    switch (opcode) {
        CASE_RETURN_STR(WS_CLOSE_REASON_NORMAL);
        CASE_RETURN_STR(WS_CLOSE_REASON_GOING_AWAY);
        CASE_RETURN_STR(WS_CLOSE_REASON_PROTOCOL_ERROR);
        CASE_RETURN_STR(WS_CLOSE_REASON_UNEXPECTED_DATA);
        CASE_RETURN_STR(WS_CLOSE_REASON_NO_REASON);
        CASE_RETURN_STR(WS_CLOSE_REASON_ABRUPTLY);
        CASE_RETURN_STR(WS_CLOSE_REASON_INCONSISTENT_DATA);
        CASE_RETURN_STR(WS_CLOSE_REASON_POLICY_VIOLATION);
        CASE_RETURN_STR(WS_CLOSE_REASON_TOO_BIG);
        CASE_RETURN_STR(WS_CLOSE_REASON_MISSING_EXTENSION);
        CASE_RETURN_STR(WS_CLOSE_REASON_SERVER_ERROR);
        CASE_RETURN_STR(WS_CLOSE_REASON_IANA_REGISTRY_START);
        CASE_RETURN_STR(WS_CLOSE_REASON_IANA_REGISTRY_END);
        CASE_RETURN_STR(WS_CLOSE_REASON_PRIVATE_START);
        CASE_RETURN_STR(WS_CLOSE_REASON_PRIVATE_END);
    default:
        if (opcode > WS_CLOSE_REASON_PRIVATE_START
            && opcode < WS_CLOSE_REASON_PRIVATE_END)
        {
            return "WS_CLOSE_REASON_PRIVATE";
        }
        return "WS_CLOSE_REASON_UNKNOWN";
    }
}

static const char *
_ws_status_print(enum ws_status status)
{
    switch (status) {
        CASE_RETURN_STR(WS_DISCONNECTED);
        CASE_RETURN_STR(WS_CONNECTED);
        CASE_RETURN_STR(WS_DISCONNECTING);
        CASE_RETURN_STR(WS_CONNECTING);
    default:
        return NULL;
    }
}

static CURL *_ws_cws_new(struct websockets *ws, const char ws_protocols[]);

static void
_ws_set_status_nolock(struct websockets *ws, enum ws_status status)
{
    if ((WS_DISCONNECTING == ws->status) && (status != WS_DISCONNECTED)) {
        return;
    }

    switch (status) {
    case WS_DISCONNECTED:
        if (WS_DISCONNECTED == ws->status) {
            logconf_info(
                &ws->conf,
                "Client is already disconnected, skipping WS_DISCONNECT");
            break;
        }
        else if (WS_DISCONNECTING == ws->status) {
            logconf_warn(&ws->conf,
                         "Abruptly changed status to WS_DISCONNECTED "
                         "(previous status: %s)",
                         _ws_status_print(ws->status));
        }

        logconf_info(&ws->conf, "Change status to WS_DISCONNECTED");
        break;
    case WS_CONNECTED:
        VASSERT_S(WS_CONNECTING == ws->status,
                  "[%s] Missing ws_start() before the event loop (Current "
                  "status: %s)",
                  ws->conf.id, _ws_status_print(ws->status));

        logconf_info(&ws->conf, "Change status to WS_CONNECTED");
        break;
    case WS_DISCONNECTING:
        if (WS_DISCONNECTED == ws->status) {
            logconf_info(
                &ws->conf,
                "Client is already disconnected, skipping WS_DISCONNECTING");
            break;
        }

        logconf_info(&ws->conf, "Change status to WS_DISCONNECTING");
        break;
    case WS_CONNECTING: /* triggered at ws_start() */
        logconf_info(&ws->conf, "Change status to WS_CONNECTING");
        break;
    default:
        ERR("[%s] Unknown ws_status (code: %d)", ws->conf.id, status);
        break;
    }
    ws->status = status;
}

static void
_ws_set_status(struct websockets *ws, enum ws_status status)
{
    pthread_mutex_lock(&ws->lock);
    _ws_set_status_nolock(ws, status);
    pthread_mutex_unlock(&ws->lock);
}

static void
cws_on_connect_cb(void *p_ws, CURL *ehandle, const char *ws_protocols)
{
    struct websockets *ws = p_ws;
    (void)ehandle;

    _ws_set_status(ws, WS_CONNECTED);

    logconf_http(
        &ws->conf, &ws->info.loginfo, ws->base_url,
        (struct logconf_szbuf){ "", 0 },
        (struct logconf_szbuf){ (char *)ws_protocols, strlen(ws_protocols) },
        "WS_RCV_CONNECT");

    logconf_trace(
        &ws->conf,
        ANSICOLOR(
            "RCV",
            ANSI_FG_YELLOW) " CONNECT (WS-Protocols: '%s') [@@@_%zu_@@@]",
        ws_protocols, ws->info.loginfo.rcounter);

    if (ws->cbs.on_connect)
        ws->cbs.on_connect(ws->cbs.data, ws, &ws->info, ws_protocols);
}

static void
cws_on_close_cb(void *p_ws,
                CURL *ehandle,
                enum cws_close_reason cwscode,
                const char *reason,
                size_t len)
{
    struct websockets *ws = p_ws;
    struct logconf_szbuf logheader = { "", 0 };
    struct logconf_szbuf logbody = { (char *)reason, len };
    (void)ehandle;

    _ws_set_status(ws, WS_DISCONNECTING);

    logconf_http(&ws->conf, &ws->info.loginfo, ws->base_url, logheader,
                 logbody, "WS_RCV_CLOSE(%d)", cwscode);

    logconf_trace(
        &ws->conf,
        ANSICOLOR("RCV",
                  ANSI_FG_YELLOW) " CLOSE(%d) (%zu bytes) [@@@_%zu_@@@]",
        cwscode, len, ws->info.loginfo.rcounter);

    if (ws->cbs.on_close)
        ws->cbs.on_close(ws->cbs.data, ws, &ws->info,
                         (enum ws_close_reason)cwscode, reason, len);

    ws->action = WS_ACTION_END_CLOSE;

    /* will set status to WS_DISCONNECTED when is_running == false */
}

static void
cws_on_text_cb(void *p_ws, CURL *ehandle, const char *text, size_t len)
{
    struct websockets *ws = p_ws;
    struct logconf_szbuf logheader = { "", 0 };
    struct logconf_szbuf logbody = { (char *)text, len };
    (void)ehandle;

    logconf_http(&ws->conf, &ws->info.loginfo, ws->base_url, logheader,
                 logbody, "WS_RCV_TEXT");

    logconf_trace(
        &ws->conf,
        ANSICOLOR("RCV", ANSI_FG_YELLOW) " TEXT (%zu bytes) [@@@_%zu_@@@]",
        len, ws->info.loginfo.rcounter);

    if (ws->cbs.on_text)
        ws->cbs.on_text(ws->cbs.data, ws, &ws->info, text, len);
}

static void
cws_on_binary_cb(void *p_ws, CURL *ehandle, const void *mem, size_t len)
{
    struct websockets *ws = p_ws;
    struct logconf_szbuf logheader = { "", 0 };
    struct logconf_szbuf logbody = { (char *)mem, len };
    (void)ehandle;

    logconf_http(&ws->conf, &ws->info.loginfo, ws->base_url, logheader,
                 logbody, "WS_RCV_BINARY");

    logconf_trace(
        &ws->conf,
        ANSICOLOR("RCV", ANSI_FG_YELLOW) " BINARY (%zu bytes) [@@@_%zu_@@@]",
        len, ws->info.loginfo.rcounter);

    if (ws->cbs.on_binary)
        ws->cbs.on_binary(ws->cbs.data, ws, &ws->info, mem, len);
}

static void
cws_on_ping_cb(void *p_ws, CURL *ehandle, const char *reason, size_t len)
{
    struct websockets *ws = p_ws;
    (void)ehandle;
#if 0
  struct logconf_szbuf logheader = { "", 0 };
  struct logconf_szbuf logbody   = { (char *)reason, len };

  logconf_http(&ws->conf, &ws->info.loginfo, ws->base_url, logheader, logbody,
               "WS_RCV_PING");
#endif

    logconf_trace(
        &ws->conf,
        ANSICOLOR("RCV", ANSI_FG_YELLOW) " PING (%zu bytes) [@@@_%zu_@@@]",
        len, ws->info.loginfo.rcounter);

    if (ws->cbs.on_ping)
        ws->cbs.on_ping(ws->cbs.data, ws, &ws->info, reason, len);
}

static void
cws_on_pong_cb(void *p_ws, CURL *ehandle, const char *reason, size_t len)
{
    struct websockets *ws = p_ws;
    (void)ehandle;
#if 0
  struct logconf_szbuf logheader = { "", 0 };
  struct logconf_szbuf logbody   = { (char *)reason, len };

  logconf_http(&ws->conf, &ws->info.loginfo, ws->base_url, logheader, logbody,
               "WS_RCV_PONG");
#endif

    logconf_trace(
        &ws->conf,
        ANSICOLOR("RCV", ANSI_FG_YELLOW) " PONG (%zu bytes) [@@@_%zu_@@@]",
        len, ws->info.loginfo.rcounter);

    if (ws->cbs.on_pong)
        ws->cbs.on_pong(ws->cbs.data, ws, &ws->info, reason, len);
}

static bool _ws_close(struct websockets *ws,
                      enum ws_close_reason code,
                      const char reason[]);

static int
_ws_check_action_cb(void *p_userdata,
                    curl_off_t dltotal,
                    curl_off_t dlnow,
                    curl_off_t ultotal,
                    curl_off_t ulnow)
{
    struct websockets *ws = p_userdata;
    int ret;
    (void)dltotal;
    (void)dlnow;
    (void)ultotal;
    (void)ulnow;

    pthread_mutex_lock(&ws->lock);
    switch (ws->action) {
    case WS_ACTION_BEGIN_CLOSE:
        logconf_warn(&ws->conf,
                     "Received pending %s, closing the connection ...",
                     ws_close_opcode_print(ws->pending_close.code));

        _ws_close(ws, ws->pending_close.code, ws->pending_close.reason);
    /* fall-through */
    case WS_ACTION_NONE:
    default:
        ret = 0;
        break;
    case WS_ACTION_END_CLOSE:
        /* END WEBSOCKETS CONNECTION */
        ret = 1;
        break;
    }
    ws->action = WS_ACTION_NONE;
    pthread_mutex_unlock(&ws->lock);

    return ret;
}

/* init easy handle with some default opt */
static CURL *
_ws_cws_new(struct websockets *ws, const char ws_protocols[])
{
    struct cws_callbacks cws_cbs = { .on_connect = &cws_on_connect_cb,
                                     .on_text = &cws_on_text_cb,
                                     .on_binary = &cws_on_binary_cb,
                                     .on_ping = &cws_on_ping_cb,
                                     .on_pong = &cws_on_pong_cb,
                                     .on_close = &cws_on_close_cb,
                                     .data = ws };

    CURL *new_ehandle = cws_new(ws->base_url, ws_protocols, &cws_cbs);

    /* set error buffer for capturing CURL error descriptions */
    curl_easy_setopt(new_ehandle, CURLOPT_ERRORBUFFER, ws->errbuf);
    /* enable follow redirections */
    curl_easy_setopt(new_ehandle, CURLOPT_FOLLOWLOCATION, 1L);
    /* enable progress function (a callback that executes constantly) */
    curl_easy_setopt(new_ehandle, CURLOPT_XFERINFOFUNCTION,
                     &_ws_check_action_cb);
    curl_easy_setopt(new_ehandle, CURLOPT_XFERINFODATA, ws);
    curl_easy_setopt(new_ehandle, CURLOPT_NOPROGRESS, 0L);

    curl_easy_setopt(new_ehandle, CURLOPT_DEBUGFUNCTION, _ws_curl_tls_check);
    curl_easy_setopt(new_ehandle, CURLOPT_VERBOSE, 1L);
    curl_easy_setopt(new_ehandle, CURLOPT_DEBUGDATA, ws);

    logconf_info(&ws->conf, "Initialized WebSockets handler");

    return new_ehandle;
}

static bool
_ws_close(struct websockets *ws,
          enum ws_close_reason code,
          const char reason[])
{
    struct logconf_szbuf logheader = { "", 0 };
    struct logconf_szbuf logbody = { (char *)reason, strlen(reason) };

    if (WS_DISCONNECTED == ws->status) {
        logconf_warn(
            &ws->conf,
            ANSICOLOR("Failed",
                      ANSI_FG_RED) " at SEND CLOSE : Connection already "
                                   "closed [@@@_%zu_@@@]",
            ws->info.loginfo.rcounter);

        return false;
    }
    if (WS_DISCONNECTING == ws->status) {
        logconf_warn(
            &ws->conf,
            ANSICOLOR("Failed",
                      ANSI_FG_RED) " at SEND CLOSE : Close already taking "
                                   "place [@@@_%zu_@@@]",
            ws->info.loginfo.rcounter);

        return false;
    }
    _ws_set_status_nolock(ws, WS_DISCONNECTING);

    if (!cws_close(ws->ehandle, (enum cws_close_reason)code, reason, SIZE_MAX))
    {
        logconf_error(
            &ws->conf,
            ANSICOLOR("Failed",
                      ANSI_FG_RED) " at SEND CLOSE(%d): %s [@@@_%zu_@@@]",
            code, reason, ws->info.loginfo.rcounter);

        return false;
    }

    logconf_http(&ws->conf, &ws->info.loginfo, ws->base_url, logheader,
                 logbody, "WS_SEND_CLOSE(%d)", code);

    logconf_trace(&ws->conf,
                  ANSICOLOR("SEND", ANSI_FG_GREEN) " CLOSE (%s) [@@@_%zu_@@@]",
                  reason, ws->info.loginfo.rcounter);

    return true;
}

enum ws_status
ws_get_status(struct websockets *ws)
{
    enum ws_status status;

    pthread_mutex_lock(&ws->lock);
    status = ws->status;
    pthread_mutex_unlock(&ws->lock);

    return status;
}

static void
default_on_ping(void *a,
                struct websockets *ws,
                struct ws_info *info,
                const char *reason,
                size_t len)
{
    (void)a;
    (void)info;
    ws_pong(ws, &ws->info, reason, len);
}

struct websockets *
ws_init(struct ws_callbacks *cbs, CURLM *mhandle, struct ws_attr *attr)
{
    struct logconf conf = {};
    struct websockets *new_ws;

    if (attr) {
        conf = *attr->conf;
    }
    conf.intercept_cb = cbs->on_log_entry;

    new_ws = calloc(1, sizeof *new_ws);
    logconf_branch(&new_ws->conf, attr ? &conf : NULL, "WEBSOCKETS");

    if (cbs) new_ws->cbs = *cbs;
    new_ws->mhandle = mhandle;

    /** respond ping with a pong by default */
    if (!new_ws->cbs.on_ping) new_ws->cbs.on_ping = &default_on_ping;

    if (pthread_mutex_init(&new_ws->lock, NULL))
        ERR("[%s] Couldn't initialize pthread mutex", new_ws->conf.id);
    if (pthread_rwlock_init(&new_ws->rwlock, NULL))
        ERR("[%s] Couldn't initialize pthread rwlock", new_ws->conf.id);

    return new_ws;
}

void
ws_set_url(struct websockets *ws,
           const char base_url[],
           const char ws_protocols[])
{
    size_t len;

    pthread_mutex_lock(&ws->lock);

    if (!*ws->base_url) {
        logconf_info(&ws->conf, "Websockets new URL: %s", base_url);
    }
    else {
        logconf_info(&ws->conf,
                     "WebSockets redirecting:\n\tfrom: %s\n\tto: %s",
                     ws->base_url, base_url);
    }

    len = snprintf(ws->base_url, sizeof(ws->base_url), "%s", base_url);
    VASSERT_S(len < sizeof(ws->base_url), "[%s] Out of bounds write attempt",
              ws->conf.id);

    if (!ws_protocols || !*ws_protocols) {
        len =
            snprintf(ws->protocols, sizeof(ws->protocols), "%s", ws_protocols);
        VASSERT_S(len < sizeof(ws->protocols),
                  "[%s] Out of bounds write attempt", ws->conf.id);
    }

    pthread_mutex_unlock(&ws->lock);
}

void
ws_cleanup(struct websockets *ws)
{
    if (ws->ehandle) cws_free(ws->ehandle);
    pthread_mutex_destroy(&ws->lock);
    pthread_rwlock_destroy(&ws->rwlock);
    free(ws);
}

bool
ws_send_binary(struct websockets *ws,
               struct ws_info *info,
               const char msg[],
               size_t msglen)
{
    struct logconf_szbuf logheader = { "", 0 };
    struct logconf_szbuf logbody = { (char *)msg, msglen };

    logconf_http(&ws->conf, &ws->info.loginfo, ws->base_url, logheader,
                 logbody, "WS_SEND_BINARY");

    logconf_trace(
        &ws->conf,
        ANSICOLOR("SEND", ANSI_FG_GREEN) " BINARY (%zu bytes) [@@@_%zu_@@@]",
        msglen, ws->info.loginfo.rcounter);

    if (WS_CONNECTED != ws->status) {
        logconf_error(
            &ws->conf,
            ANSICOLOR("Failed", ANSI_FG_RED) " at SEND BINARY : No active "
                                             "connection [@@@_%zu_@@@]",
            ws->info.loginfo.rcounter);
        return false;
    }

    if (info) *info = ws->info;

    if (!cws_send(ws->ehandle, false, msg, msglen)) {
        logconf_error(
            &ws->conf,
            ANSICOLOR("Failed", ANSI_FG_RED) " at SEND BINARY [@@@_%zu_@@@]",
            ws->info.loginfo.rcounter);
        return false;
    }
    return true;
}

bool
ws_send_text(struct websockets *ws,
             struct ws_info *info,
             const char text[],
             size_t len)
{
    struct logconf_szbuf logheader = { "", 0 };
    struct logconf_szbuf logbody = { (char *)text, len };

    logconf_http(&ws->conf, &ws->info.loginfo, ws->base_url, logheader,
                 logbody, "WS_SEND_TEXT");

    logconf_trace(
        &ws->conf,
        ANSICOLOR("SEND", ANSI_FG_GREEN) " TEXT (%zu bytes) [@@@_%zu_@@@]",
        len, ws->info.loginfo.rcounter);

    if (WS_CONNECTED != ws->status) {
        logconf_error(
            &ws->conf,
            ANSICOLOR("Failed", ANSI_FG_RED) " at SEND TEXT : No active "
                                             "connection [@@@_%zu_@@@]",
            ws->info.loginfo.rcounter);

        return false;
    }

    if (info) *info = ws->info;

    if (!cws_send(ws->ehandle, true, text, len)) {
        logconf_error(
            &ws->conf,
            ANSICOLOR("Failed", ANSI_FG_RED) " at SEND TEXT [@@@_%zu_@@@]",
            ws->info.loginfo.rcounter);

        return false;
    }

    return true;
}

bool
ws_ping(struct websockets *ws,
        struct ws_info *info,
        const char *reason,
        size_t len)
{
    (void)info;
#if 0
  struct logconf_szbuf logheader = { "", 0 };
  struct logconf_szbuf logbody   = { (char *)reason, len };

  logconf_http(&ws->conf, &ws->info.loginfo, ws->base_url, logheader, logbody,
               "WS_SEND_PING");
#endif

    logconf_trace(
        &ws->conf,
        ANSICOLOR("SEND", ANSI_FG_GREEN) " PING (%zu bytes) [@@@_%zu_@@@]",
        len, ws->info.loginfo.rcounter);

    if (WS_CONNECTED != ws->status) {
        logconf_error(
            &ws->conf,
            ANSICOLOR("Failed",
                      ANSI_FG_RED) " at SEND PING : No active connection");

        return false;
    }

    if (!cws_ping(ws->ehandle, reason, len)) {
        logconf_error(&ws->conf,
                      ANSICOLOR("Failed", ANSI_FG_RED) " at SEND PING.");

        return false;
    }

    return true;
}

bool
ws_pong(struct websockets *ws,
        struct ws_info *info,
        const char *reason,
        size_t len)
{
    (void)info;
#if 0
  struct logconf_szbuf logheader = { "", 0 };
  struct logconf_szbuf logbody   = { (char *)reason, len };

  logconf_http(&ws->conf, &ws->info.loginfo, ws->base_url, logheader, logbody,
               "WS_SEND_PONG");
#endif

    logconf_trace(
        &ws->conf,
        ANSICOLOR("SEND", ANSI_FG_GREEN) " PONG (%zu bytes) [@@@_%zu_@@@]",
        len, ws->info.loginfo.rcounter);

    if (WS_CONNECTED != ws->status) {
        logconf_error(
            &ws->conf,
            ANSICOLOR("Failed",
                      ANSI_FG_RED) " at SEND PONG : No active connection");

        return false;
    }

    if (!cws_pong(ws->ehandle, reason, len)) {
        logconf_error(&ws->conf,
                      ANSICOLOR("Failed", ANSI_FG_RED) " at SEND PONG.");

        return false;
    }

    return true;
}

CURL *
ws_start(struct websockets *ws)
{
    memset(&ws->pending_close, 0, sizeof ws->pending_close);
    ws->action = WS_ACTION_NONE;

    VASSERT_S(!ws_is_alive(ws),
              "[%s] Please shutdown current WebSockets connection before "
              "calling ws_start() (Current status: %s)",
              ws->conf.id, _ws_status_print(ws->status));

    VASSERT_S(NULL == ws->ehandle,
              "[%s] (Internal error) Attempt to reconnect without properly "
              "closing the connection",
              ws->conf.id);

    if (!ws->ehandle) ws->ehandle = _ws_cws_new(ws, ws->protocols);
    curl_multi_add_handle(ws->mhandle, ws->ehandle);

    _ws_set_status(ws, WS_CONNECTING);

    return ws->ehandle;
}

// TODO: split the cleanup logic from the shutdown logic
void
ws_end(struct websockets *ws)
{
    struct CURLMsg *curlmsg;
    int msgq = 0;

    /* tag as disconnecting */
    _ws_set_status(ws, WS_DISCONNECTING);

    /* read messages/informationals from the individual transfers */
    curlmsg = curl_multi_info_read(ws->mhandle, &msgq);
    if (curlmsg && ws->ehandle == curlmsg->easy_handle) {
        CURLcode ecode;

        switch (ecode = curlmsg->data.result) {
        case CURLE_OK:
        case CURLE_ABORTED_BY_CALLBACK: /* _ws_check_action_cb() */
            logconf_info(&ws->conf, "Disconnected gracefully");
            break;
        case CURLE_READ_ERROR:
        default:
            logconf_error(&ws->conf, "(CURLE code: %d) %s", ecode,
                          !*ws->errbuf ? curl_easy_strerror(ecode)
                                       : ws->errbuf);
            logconf_error(&ws->conf, "Disconnected abruptly");
            break;
        }

        curl_multi_remove_handle(ws->mhandle, ws->ehandle);
    }

    /* reset for next iteration */
    *ws->errbuf = '\0';
    if (ws->ehandle) {
        cws_free(ws->ehandle);
        ws->ehandle = NULL;
    }

    _ws_set_status(ws, WS_DISCONNECTED);
}

bool
ws_easy_run(struct websockets *ws, uint64_t wait_ms, uint64_t *tstamp)
{
    int is_running = 0;
    CURLMcode mcode;
    /* unused but curl_multi_wait() demands it */
    int numfds = 0;

    /** update WebSockets concept of "now" */
    *tstamp = ws_timestamp_update(ws);

    /**
     * Perform Read/Write pending sockets activity (if any)
     * @note ws_close() and ws_send_text() are example of pending
     *        write activities
     * @note Callbacks such as ws_on_text(), ws_on_ping(), etc are
     *        example of pending read activities
     * @note Its worth noting that all websockets.c callbacks are
     *        inherently single-threaded. websockets.c doesn't create
     *        new threads.
     */
    if (CURLM_OK == (mcode = curl_multi_perform(ws->mhandle, &is_running))) {
        /* wait for some activity or timeout after "wait_ms" elapsed */
        mcode = curl_multi_wait(ws->mhandle, NULL, 0, wait_ms, &numfds);
    }

    if (mcode != CURLM_OK) CURLM_LOG(ws, mcode);

    return is_running != 0;
}

bool
ws_multi_socket_run(struct websockets *ws, uint64_t *tstamp)
{
    int is_running = 0;
    CURLMcode mcode;

    /** update WebSockets concept of "now" */
    *tstamp = ws_timestamp_update(ws);

    mcode = curl_multi_socket_all(ws->mhandle, &is_running);

    if (mcode != CURLM_OK) CURLM_LOG(ws, mcode);

    return is_running != 0;
}

uint64_t
ws_timestamp(struct websockets *ws)
{
    uint64_t now_tstamp;

    pthread_rwlock_rdlock(&ws->rwlock);
    now_tstamp = ws->now_tstamp;
    pthread_rwlock_unlock(&ws->rwlock);

    return now_tstamp;
}

uint64_t
ws_timestamp_update(struct websockets *ws)
{
    uint64_t now_tstamp;

    pthread_rwlock_wrlock(&ws->rwlock);
    now_tstamp = ws->now_tstamp = cog_timestamp_ms();
    pthread_rwlock_unlock(&ws->rwlock);

    return now_tstamp;
}

void
ws_close(struct websockets *ws,
         const enum ws_close_reason code,
         const char reason[],
         const size_t len)
{
    logconf_warn(&ws->conf,
                 "Attempting to close WebSockets connection with %s : %.*s",
                 ws_close_opcode_print(code), (int)len, reason);

    pthread_mutex_lock(&ws->lock);
    ws->action = WS_ACTION_BEGIN_CLOSE;
    ws->pending_close.code = code;

    snprintf(ws->pending_close.reason, sizeof(ws->pending_close.reason),
             "%.*s", (int)len, reason);

    pthread_mutex_unlock(&ws->lock);
}

void
ws_add_header(struct websockets *ws, const char field[], const char value[])
{
    ASSERT_S(ws_is_alive(ws),
             "ws_start() must have been called prior to ws_add_header()");
    cws_add_header(ws->ehandle, field, value);
}
