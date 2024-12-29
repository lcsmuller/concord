#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "discord.h"
#include "discord-internal.h"
#include "discord-worker.h"
#include "osname.h"

/* return enumerator as string in case of a match */
#define CASE_RETURN_STR(code)                                                 \
    case code:                                                                \
        return #code

static const char *
_discord_gateway_opcode_print(enum discord_gateway_opcodes opcode)
{
    switch (opcode) {
        CASE_RETURN_STR(DISCORD_GATEWAY_DISPATCH);
        CASE_RETURN_STR(DISCORD_GATEWAY_HEARTBEAT);
        CASE_RETURN_STR(DISCORD_GATEWAY_IDENTIFY);
        CASE_RETURN_STR(DISCORD_GATEWAY_PRESENCE_UPDATE);
        CASE_RETURN_STR(DISCORD_GATEWAY_VOICE_STATE_UPDATE);
        CASE_RETURN_STR(DISCORD_GATEWAY_RESUME);
        CASE_RETURN_STR(DISCORD_GATEWAY_RECONNECT);
        CASE_RETURN_STR(DISCORD_GATEWAY_REQUEST_GUILD_MEMBERS);
        CASE_RETURN_STR(DISCORD_GATEWAY_INVALID_SESSION);
        CASE_RETURN_STR(DISCORD_GATEWAY_HELLO);
        CASE_RETURN_STR(DISCORD_GATEWAY_HEARTBEAT_ACK);
    default:
        return "INVALID_GATEWAY_OPCODE";
    }
}

static const char *
_discord_gateway_close_opcode_print(enum discord_gateway_close_opcodes opcode)
{
    switch (opcode) {
        CASE_RETURN_STR(DISCORD_GATEWAY_CLOSE_REASON_UNKNOWN_ERROR);
        CASE_RETURN_STR(DISCORD_GATEWAY_CLOSE_REASON_UNKNOWN_OPCODE);
        CASE_RETURN_STR(DISCORD_GATEWAY_CLOSE_REASON_DECODE_ERROR);
        CASE_RETURN_STR(DISCORD_GATEWAY_CLOSE_REASON_NOT_AUTHENTICATED);
        CASE_RETURN_STR(DISCORD_GATEWAY_CLOSE_REASON_AUTHENTICATION_FAILED);
        CASE_RETURN_STR(DISCORD_GATEWAY_CLOSE_REASON_ALREADY_AUTHENTICATED);
        CASE_RETURN_STR(DISCORD_GATEWAY_CLOSE_REASON_INVALID_SEQUENCE);
        CASE_RETURN_STR(DISCORD_GATEWAY_CLOSE_REASON_RATE_LIMITED);
        CASE_RETURN_STR(DISCORD_GATEWAY_CLOSE_REASON_SESSION_TIMED_OUT);
        CASE_RETURN_STR(DISCORD_GATEWAY_CLOSE_REASON_INVALID_SHARD);
        CASE_RETURN_STR(DISCORD_GATEWAY_CLOSE_REASON_SHARDING_REQUIRED);
        CASE_RETURN_STR(DISCORD_GATEWAY_CLOSE_REASON_INVALID_API_VERSION);
        CASE_RETURN_STR(DISCORD_GATEWAY_CLOSE_REASON_INVALID_INTENTS);
        CASE_RETURN_STR(DISCORD_GATEWAY_CLOSE_REASON_DISALLOWED_INTENTS);
        CASE_RETURN_STR(DISCORD_GATEWAY_CLOSE_REASON_RECONNECT);
    default: {
        const char *str = ws_close_opcode_print((enum ws_close_reason)opcode);
        if (str) return str;

        log_warn("Unknown WebSockets close opcode (code: %d)", opcode);
    }
        return "UNKNOWN_WEBSOCKETS_CLOSE_OPCODE";
    }
}

static void
_discord_on_hello(struct discord_gateway *gw)
{
    jsmnf_pair *f;

    if ((f = jsmnf_find(gw->payload.data, gw->payload.json.start,
                        "heartbeat_interval", 18)))
        gw->timer->hbeat_interval =
            strtoll(gw->payload.json.start + f->v.pos, NULL, 10);

    if (gw->session->status & DISCORD_SESSION_RESUMABLE)
        discord_gateway_send_resume(gw, &(struct discord_resume){
                                            .token = gw->id.token,
                                            .session_id = gw->session->id,
                                            .seq = gw->payload.seq,
                                        });
    else
        discord_gateway_send_identify(gw, &gw->id);
}

#define RETURN_IF_MATCH(event, str)                                           \
    if (!strcmp(#event, str)) return DISCORD_EV_##event

static enum discord_gateway_events
_discord_gateway_event_eval(const char name[])
{
    RETURN_IF_MATCH(READY, name);
    RETURN_IF_MATCH(RESUMED, name);
    RETURN_IF_MATCH(RECONNECT, name);
    RETURN_IF_MATCH(INVALID_SESSION, name);
    RETURN_IF_MATCH(APPLICATION_COMMAND_PERMISSIONS_UPDATE, name);
    RETURN_IF_MATCH(AUTO_MODERATION_RULE_CREATE, name);
    RETURN_IF_MATCH(AUTO_MODERATION_RULE_UPDATE, name);
    RETURN_IF_MATCH(AUTO_MODERATION_RULE_DELETE, name);
    RETURN_IF_MATCH(AUTO_MODERATION_ACTION_EXECUTION, name);
    RETURN_IF_MATCH(CHANNEL_CREATE, name);
    RETURN_IF_MATCH(CHANNEL_UPDATE, name);
    RETURN_IF_MATCH(CHANNEL_DELETE, name);
    RETURN_IF_MATCH(CHANNEL_PINS_UPDATE, name);
    RETURN_IF_MATCH(THREAD_CREATE, name);
    RETURN_IF_MATCH(THREAD_UPDATE, name);
    RETURN_IF_MATCH(THREAD_DELETE, name);
    RETURN_IF_MATCH(THREAD_LIST_SYNC, name);
    RETURN_IF_MATCH(THREAD_MEMBER_UPDATE, name);
    RETURN_IF_MATCH(THREAD_MEMBERS_UPDATE, name);
    RETURN_IF_MATCH(GUILD_CREATE, name);
    RETURN_IF_MATCH(GUILD_UPDATE, name);
    RETURN_IF_MATCH(GUILD_DELETE, name);
    RETURN_IF_MATCH(GUILD_BAN_ADD, name);
    RETURN_IF_MATCH(GUILD_BAN_REMOVE, name);
    RETURN_IF_MATCH(GUILD_EMOJIS_UPDATE, name);
    RETURN_IF_MATCH(GUILD_STICKERS_UPDATE, name);
    RETURN_IF_MATCH(GUILD_INTEGRATIONS_UPDATE, name);
    RETURN_IF_MATCH(GUILD_MEMBER_ADD, name);
    RETURN_IF_MATCH(GUILD_MEMBER_UPDATE, name);
    RETURN_IF_MATCH(GUILD_MEMBER_REMOVE, name);
    RETURN_IF_MATCH(GUILD_MEMBERS_CHUNK, name);
    RETURN_IF_MATCH(GUILD_ROLE_CREATE, name);
    RETURN_IF_MATCH(GUILD_ROLE_UPDATE, name);
    RETURN_IF_MATCH(GUILD_ROLE_DELETE, name);
    RETURN_IF_MATCH(GUILD_SCHEDULED_EVENT_CREATE, name);
    RETURN_IF_MATCH(GUILD_SCHEDULED_EVENT_UPDATE, name);
    RETURN_IF_MATCH(GUILD_SCHEDULED_EVENT_DELETE, name);
    RETURN_IF_MATCH(GUILD_SCHEDULED_EVENT_USER_ADD, name);
    RETURN_IF_MATCH(GUILD_SCHEDULED_EVENT_USER_REMOVE, name);
    RETURN_IF_MATCH(INTEGRATION_CREATE, name);
    RETURN_IF_MATCH(INTEGRATION_UPDATE, name);
    RETURN_IF_MATCH(INTEGRATION_DELETE, name);
    RETURN_IF_MATCH(INTERACTION_CREATE, name);
    RETURN_IF_MATCH(INVITE_CREATE, name);
    RETURN_IF_MATCH(INVITE_DELETE, name);
    RETURN_IF_MATCH(MESSAGE_CREATE, name);
    RETURN_IF_MATCH(MESSAGE_UPDATE, name);
    RETURN_IF_MATCH(MESSAGE_DELETE, name);
    RETURN_IF_MATCH(MESSAGE_DELETE_BULK, name);
    RETURN_IF_MATCH(MESSAGE_REACTION_ADD, name);
    RETURN_IF_MATCH(MESSAGE_REACTION_REMOVE, name);
    RETURN_IF_MATCH(MESSAGE_REACTION_REMOVE_ALL, name);
    RETURN_IF_MATCH(MESSAGE_REACTION_REMOVE_EMOJI, name);
    RETURN_IF_MATCH(PRESENCE_UPDATE, name);
    RETURN_IF_MATCH(STAGE_INSTANCE_CREATE, name);
    RETURN_IF_MATCH(STAGE_INSTANCE_DELETE, name);
    RETURN_IF_MATCH(STAGE_INSTANCE_UPDATE, name);
    RETURN_IF_MATCH(TYPING_START, name);
    RETURN_IF_MATCH(USER_UPDATE, name);
    RETURN_IF_MATCH(VOICE_STATE_UPDATE, name);
    RETURN_IF_MATCH(VOICE_SERVER_UPDATE, name);
    RETURN_IF_MATCH(WEBHOOKS_UPDATE, name);
    return DISCORD_EV_NONE;
}

#undef RETURN_IF_MATCH

static struct discord_gateway *
_discord_gateway_clone(const struct discord_gateway *gw)
{
    return &discord_clone(CLIENT(gw, gw))->gw;
}

static void
_discord_gateway_clone_cleanup(struct discord_gateway *clone)
{
    discord_cleanup(CLIENT(clone, gw));
}

static void
_discord_gateway_dispatch_thread(void *p_gw)
{
    struct discord_gateway *gw = p_gw;

    logconf_info(&gw->conf,
                 "Thread " ANSICOLOR("starts", ANSI_FG_RED) " to serve %s",
                 gw->payload.name);

    discord_gateway_dispatch(gw);

    logconf_info(&gw->conf,
                 "Thread " ANSICOLOR("exits", ANSI_FG_RED) " from serving %s",
                 gw->payload.name);

    _discord_gateway_clone_cleanup(gw);
}

static void
_discord_on_dispatch(struct discord_gateway *gw)
{
    struct discord *client = CLIENT(gw, gw);

    /* TODO: this should only apply for user dispatched payloads? */
#if 0
  /* Ratelimit check */
  if (gw->timer->now - gw->timer->event_last < 60000) {
    ++gw->session->event_count;
    ASSERT_S(gw->session->event_count < 120,
             "Reach event dispatch threshold (120 every 60 seconds)");
  }
  else {
    gw->timer->event_last = gw->timer->now;
    gw->session->event_count = 0;
  }
#endif

    switch (gw->payload.event) {
    case DISCORD_EV_READY: {
        jsmnf_pair *f;

        logconf_info(&gw->conf, "Successfully started a Discord session!");

        if ((f = jsmnf_find(gw->payload.data, gw->payload.json.start,
                            "session_id", 10)))
            snprintf(gw->session->id, sizeof(gw->session->id), "%.*s",
                     (int)f->v.len, gw->payload.json.start + f->v.pos);
        ASSERT_S(*gw->session->id, "Missing session_id from READY event");

        if ((f = jsmnf_find(gw->payload.data, gw->payload.json.start,
                            "resume_gateway_url", 18)))
        {
            const char *url = gw->payload.json.start + f->v.pos;
            int url_len = (int)f->v.len;

            url_len = snprintf(gw->session->resume_url,
                               sizeof(gw->session->resume_url),
                               "%.*s%s" DISCORD_GATEWAY_URL_SUFFIX, url_len,
                               url, ('/' == url[url_len - 1]) ? "" : "/");
            ASSERT_NOT_OOB(url_len, sizeof(gw->session->resume_url));
        }

        gw->session->is_ready = true;
        gw->session->retry.attempt = 0;

        discord_gateway_send_heartbeat(gw, gw->payload.seq);
    } break;
    case DISCORD_EV_RESUMED:
        logconf_info(&gw->conf, "Succesfully resumed a Discord session!");

        gw->session->is_ready = true;
        gw->session->retry.attempt = 0;

        if (client->cache.on_shard_resumed)
            client->cache.on_shard_resumed(client, &gw->id);
        discord_gateway_send_heartbeat(gw, gw->payload.seq);
        break;
    default:
        break;
    }

    /* get dispatch event opcode */
    enum discord_event_scheduler mode =
        gw->scheduler(client, gw->payload.json.start + gw->payload.data->v.pos,
                      gw->payload.data->v.len, gw->payload.event);

    switch (mode) {
    case DISCORD_EVENT_IGNORE:
        break;
    case DISCORD_EVENT_MAIN_THREAD:
        discord_gateway_dispatch(gw);
        break;
    case DISCORD_EVENT_WORKER_THREAD: {
        struct discord_gateway *clone = _discord_gateway_clone(gw);
        CCORDcode code = discord_worker_add(
            client, &_discord_gateway_dispatch_thread, clone);

        if (code != CCORD_OK) {
            log_error("Couldn't start worker-thread (code %d)", code);
            _discord_gateway_clone_cleanup(clone);
        }
    } break;
    default:
        ERR("Unknown event handling mode (code: %d)", mode);
    }
}

static void
_discord_on_invalid_session(struct discord_gateway *gw)
{
    enum ws_close_reason opcode;
    const char *reason;

    /* attempt to resume if session isn't invalid */
    gw->session->retry.enable = true;
    gw->session->status = DISCORD_SESSION_SHUTDOWN;
    if (gw->payload.data->v.len != 5
        || strncmp("false", gw->payload.json.start + gw->payload.data->v.pos,
                   5))
    {
        gw->session->status |= DISCORD_SESSION_RESUMABLE;
        reason = "Invalid session, will attempt to resume";
        opcode = (enum ws_close_reason)DISCORD_GATEWAY_CLOSE_REASON_RECONNECT;
    }
    else {
        reason = "Invalid session, can't resume";
        opcode = WS_CLOSE_REASON_NORMAL;
    }

    ws_close(gw->ws, opcode, reason, SIZE_MAX);
    io_poller_curlm_enable_perform(CLIENT(gw, gw)->io_poller, gw->mhandle);
}

static void
_discord_on_reconnect(struct discord_gateway *gw)
{
    const char reason[] = "Discord expects client to reconnect";

    gw->session->status = DISCORD_SESSION_RESUMABLE | DISCORD_SESSION_SHUTDOWN;
    gw->session->retry.enable = true;

    ws_close(gw->ws,
             (enum ws_close_reason)DISCORD_GATEWAY_CLOSE_REASON_RECONNECT,
             reason, sizeof(reason));
    io_poller_curlm_enable_perform(CLIENT(gw, gw)->io_poller, gw->mhandle);
}

static void
_discord_on_heartbeat_ack(struct discord_gateway *gw)
{
    /* get request / response interval in milliseconds */
    pthread_rwlock_wrlock(&gw->timer->rwlock);
    gw->timer->ping_ms = (int)(gw->timer->now - gw->timer->hbeat_last);
    gw->timer->hbeat_acknowledged = true;
    pthread_rwlock_unlock(&gw->timer->rwlock);

    logconf_trace(&gw->conf, "PING: %d ms", gw->timer->ping_ms);
}

static void
_ws_on_connect(void *p_gw,
               struct websockets *ws,
               struct ws_info *info)
{
    (void)ws;
    (void)info;
    struct discord_gateway *gw = p_gw;

    logconf_info(&gw->conf, "Connected");
}

static void
_ws_on_close(void *p_gw,
             struct websockets *ws,
             struct ws_info *info,
             enum ws_close_reason wscode,
             const char *reason,
             size_t len)
{
    (void)ws;
    (void)info;
    struct discord_gateway *gw = p_gw;
    enum discord_gateway_close_opcodes opcode =
        (enum discord_gateway_close_opcodes)wscode;

    logconf_warn(
        &gw->conf,
        ANSICOLOR("CLOSE %s", ANSI_FG_RED) " (code: %4d, %zu bytes): '%.*s'",
        _discord_gateway_close_opcode_print(opcode), opcode, len, (int)len,
        reason);

    /* user-triggered shutdown */
    if (gw->session->status & DISCORD_SESSION_SHUTDOWN) {
        if (CLIENT(gw, gw)->cache.on_shard_disconnected)
            CLIENT(gw, gw)->cache.on_shard_disconnected(
                CLIENT(gw, gw), &gw->id,
                gw->session->status & DISCORD_SESSION_RESUMABLE);
        return;
    }

    /* mark as in the process of being shutdown */
    gw->session->status |= DISCORD_SESSION_SHUTDOWN;

    switch (opcode) {
    default: /* websocket/clouflare opcodes */
        if (WS_CLOSE_REASON_NORMAL == (enum ws_close_reason)opcode) {
            gw->session->status |= DISCORD_SESSION_RESUMABLE;
            gw->session->retry.enable = true;
            break;
        }
        /* fall-through */
    case DISCORD_GATEWAY_CLOSE_REASON_INVALID_SEQUENCE:
    case DISCORD_GATEWAY_CLOSE_REASON_SESSION_TIMED_OUT:
        logconf_warn(
            &gw->conf,
            "Gateway will attempt to reconnect and start a new session");
        gw->session->status &= ~DISCORD_SESSION_RESUMABLE;
        gw->session->retry.enable = true;
        break;
    case DISCORD_GATEWAY_CLOSE_REASON_AUTHENTICATION_FAILED:
    case DISCORD_GATEWAY_CLOSE_REASON_SHARDING_REQUIRED:
    case DISCORD_GATEWAY_CLOSE_REASON_INVALID_API_VERSION:
    case DISCORD_GATEWAY_CLOSE_REASON_INVALID_INTENTS:
    case DISCORD_GATEWAY_CLOSE_REASON_INVALID_SHARD:
    case DISCORD_GATEWAY_CLOSE_REASON_DISALLOWED_INTENTS:
        logconf_warn(&gw->conf, "Gateway will not attempt to reconnect");
        gw->session->status &= ~DISCORD_SESSION_RESUMABLE;
        gw->session->retry.enable = false;
        break;
    case DISCORD_GATEWAY_CLOSE_REASON_UNKNOWN_ERROR:
    case DISCORD_GATEWAY_CLOSE_REASON_UNKNOWN_OPCODE:
    case DISCORD_GATEWAY_CLOSE_REASON_DECODE_ERROR:
    case DISCORD_GATEWAY_CLOSE_REASON_NOT_AUTHENTICATED:
    case DISCORD_GATEWAY_CLOSE_REASON_ALREADY_AUTHENTICATED:
    case DISCORD_GATEWAY_CLOSE_REASON_RATE_LIMITED:
        logconf_warn(
            &gw->conf,
            "Gateway will attempt to reconnect and resume current session");
        gw->session->status |= DISCORD_SESSION_RESUMABLE;
        gw->session->retry.enable = true;
        break;
    }
    if (CLIENT(gw, gw)->cache.on_shard_disconnected)
        CLIENT(gw, gw)->cache.on_shard_disconnected(
            CLIENT(gw, gw), &gw->id,
            gw->session->status & DISCORD_SESSION_RESUMABLE);
}

static bool
_discord_gateway_payload_from_json(struct discord_gateway_payload *payload,
                                   const char text[],
                                   size_t len)
{
    payload->json.start = (char *)text;
    payload->json.size = len;

    jsmn_parser parser;
    jsmn_init(&parser);
    if (jsmn_parse_auto(&parser, text, len, &payload->json.tokens,
                        &payload->json.ntokens)
        <= 0)
        return false;

    jsmnf_loader loader;
    jsmnf_init(&loader);
    if (jsmnf_load_auto(&loader, text, payload->json.tokens, parser.toknext,
                        &payload->json.pairs, &payload->json.npairs)
        <= 0)
        return false;

    jsmnf_pair *f;
    if ((f = jsmnf_find(payload->json.pairs, text, "t", 1))) {
        if (JSMN_STRING == f->type)
            snprintf(payload->name, sizeof(payload->name), "%.*s",
                     (int)f->v.len, text + f->v.pos);
        else
            *payload->name = '\0';

        payload->event = _discord_gateway_event_eval(payload->name);
    }
    if ((f = jsmnf_find(payload->json.pairs, text, "s", 1))) {
        int seq = (int)strtol(text + f->v.pos, NULL, 10);
        if (seq) payload->seq = seq;
    }
    if ((f = jsmnf_find(payload->json.pairs, text, "op", 2)))
        payload->opcode =
            (enum discord_gateway_opcodes)strtol(text + f->v.pos, NULL, 10);
    payload->data = jsmnf_find(payload->json.pairs, text, "d", 1);

    return true;
}

static void
_ws_on_text(void *p_gw,
            struct websockets *ws,
            struct ws_info *info,
            const char *text,
            size_t len)
{
    (void)ws;
    struct discord_gateway *gw = p_gw;

    if (!_discord_gateway_payload_from_json(&gw->payload, text, len)) {
        logconf_fatal(&gw->conf, "Couldn't parse Gateway Payload");
        return;
    }

    logconf_trace(
        &gw->conf,
        ANSICOLOR("RCV",
                  ANSI_FG_BRIGHT_YELLOW) " %s%s%s (%zu bytes) [@@@_%zu_@@@]",
        _discord_gateway_opcode_print(gw->payload.opcode),
        *gw->payload.name ? " -> " : "", gw->payload.name, len,
        info->loginfo.counter);

    switch (gw->payload.opcode) {
    case DISCORD_GATEWAY_DISPATCH:
        _discord_on_dispatch(gw);
        break;
    case DISCORD_GATEWAY_INVALID_SESSION:
        _discord_on_invalid_session(gw);
        break;
    case DISCORD_GATEWAY_RECONNECT:
        _discord_on_reconnect(gw);
        break;
    case DISCORD_GATEWAY_HELLO:
        _discord_on_hello(gw);
        break;
    case DISCORD_GATEWAY_HEARTBEAT_ACK:
        _discord_on_heartbeat_ack(gw);
        break;
    default:
        logconf_error(&gw->conf,
                      "Not yet implemented Gateway Event (code: %d)",
                      gw->payload.opcode);
        break;
    }
}

static discord_event_scheduler_t
_discord_on_scheduler_default(struct discord *a,
                              const char b[],
                              size_t c,
                              enum discord_gateway_events d)
{
    (void)a;
    (void)b;
    (void)c;
    (void)d;
    return DISCORD_EVENT_MAIN_THREAD;
}

static int
_discord_on_gateway_perform(struct io_poller *io, CURLM *mhandle, void *p_gw)
{
    (void)io;
    (void)mhandle;
    return discord_gateway_perform(p_gw);
}

void
discord_gateway_init(struct discord_gateway *gw,
                     struct logconf *conf,
                     const char token[])
{
    struct discord *client = CLIENT(gw, gw);
    /* Web-Sockets callbacks */
    struct ws_callbacks cbs = { .data = gw,
                                .on_connect = &_ws_on_connect,
                                .on_text = &_ws_on_text,
                                .on_close = &_ws_on_close };
    /* Web-Sockets custom attributes */
    struct ws_attr attr = { .conf = conf };

    /* Web-Sockets handler */
    gw->mhandle = curl_multi_init();
    io_poller_curlm_add(client->io_poller, gw->mhandle,
                        _discord_on_gateway_perform, gw);
    gw->ws = ws_init(&cbs, gw->mhandle, &attr);
    logconf_branch(&gw->conf, conf, "DISCORD_GATEWAY");

    gw->timer = calloc(1, sizeof *gw->timer);
    ASSERT_S(!pthread_rwlock_init(&gw->timer->rwlock, NULL),
             "Couldn't initialize Gateway's rwlock");

    /* mark true to not get reconnected each reconnect */
    gw->timer->hbeat_acknowledged = true;

    /* client connection status */
    gw->session = calloc(1, sizeof *gw->session);
    gw->session->retry.enable = true;
    /* default infinite retries TODO: configurable */
    gw->session->retry.limit = -1;

    /* default callbacks */
    gw->scheduler = _discord_on_scheduler_default;

    /* connection identify token */
    gw->id.token = (char *)token;
    /* connection identify properties */
    gw->id.properties = calloc(1, sizeof *gw->id.properties);
    gw->id.properties->os = OSNAME;
    gw->id.properties->browser = "concord";
    gw->id.properties->device = "concord";
    /* the bot initial presence */
    gw->id.presence = calloc(1, sizeof *gw->id.presence);
    gw->id.presence->status = "online";
    gw->id.presence->since = cog_timestamp_ms();

    discord_gateway_send_presence_update(gw, gw->id.presence);
}

void
discord_gateway_cleanup(struct discord_gateway *gw)
{
    if (gw->timer->hbeat_timer)
        discord_internal_timer_ctl(CLIENT(gw, gw),
                                   &(struct discord_timer){
                                       .id = gw->timer->hbeat_timer,
                                       .flags = DISCORD_TIMER_DELETE,
                                   });
    /* cleanup WebSockets handle */
    io_poller_curlm_del(CLIENT(gw, gw)->io_poller, gw->mhandle);
    curl_multi_cleanup(gw->mhandle);
    ws_cleanup(gw->ws);
    /* cleanup timers */
    pthread_rwlock_destroy(&gw->timer->rwlock);
    free(gw->timer);
    /* cleanup bot identification */
    free(gw->id.properties);
    free(gw->id.presence);
    /* cleanup client session */
    free(gw->session);
    if (gw->payload.json.pairs) free(gw->payload.json.pairs);
    if (gw->payload.json.tokens) free(gw->payload.json.tokens);
}

#ifdef CCORD_DEBUG_WEBSOCKETS
static void
_ws_curl_debug_dump(const char *text,
                    FILE *stream,
                    unsigned char *ptr,
                    size_t size)
{
    unsigned int width = 0x10;
    size_t i;
    size_t c;

    fprintf(stream, "%s, %10.10lu bytes (0x%8.8lx)\n", text,
            (unsigned long)size, (unsigned long)size);

    for (i = 0; i < size; i += width) {

        fprintf(stream, "%4.4lx: ", (unsigned long)i);

        for (c = 0; c < width; c++)
            if (i + c < size)
                fprintf(stream, "%02x ", ptr[i + c]);
            else
                fputs("   ", stream);

        for (c = 0; (c < width) && (i + c < size); c++) {
            /* check for 0D0A; if found, skip past and start a new line of
             * output */
            if ((i + c + 1 < size) && ptr[i + c] == 0x0D
                && ptr[i + c + 1] == 0x0A) {
                i += (c + 2 - width);
                break;
            }
            fprintf(stream, "%c",
                    (ptr[i + c] >= 0x20) && (ptr[i + c] < 0x80) ? ptr[i + c]
                                                                : '.');
            /* check again for 0D0A, to avoid an extra \n if it's at width */
            if ((i + c + 2 < size) && ptr[i + c + 1] == 0x0D
                && ptr[i + c + 2] == 0x0A) {
                i += (c + 3 - width);
                break;
            }
        }
        fputc('\n', stream); /* newline */
    }
    fflush(stream);
}

static int
_ws_curl_debug_trace(
    CURL *handle, curl_infotype type, char *data, size_t size, void *userp)
{
    (void)handle;
    (void)userp;
    const char *text;

    switch (type) {
    case CURLINFO_TEXT:
        fprintf(stderr, "== Info: %s", data);
        /* FALLTHROUGH */
    default:
        return 0;

    case CURLINFO_HEADER_OUT:
        text = "=> Send header";
        break;
    case CURLINFO_DATA_OUT:
        text = "=> Send data";
        break;
    case CURLINFO_SSL_DATA_OUT:
        text = "=> Send SSL data";
        break;
    case CURLINFO_HEADER_IN:
        text = "<= Recv header";
        break;
    case CURLINFO_DATA_IN:
        text = "<= Recv data";
        break;
    case CURLINFO_SSL_DATA_IN:
        text = "<= Recv SSL data";
        break;
    }

    _ws_curl_debug_dump(text, stderr, (unsigned char *)data, size);
    return 0;
}
#endif /* CCORD_DEBUG_WEBSOCKETS */

static bool
_discord_gateway_session_from_json(struct discord_gateway_session *session,
                                   const char text[],
                                   size_t len)
{
    jsmn_parser parser;
    jsmntok_t tokens[32];
    jsmn_init(&parser);
    if (jsmn_parse(&parser, text, len, tokens, sizeof(tokens) / sizeof *tokens)
        <= 0)
        return false;

    jsmnf_loader loader;
    jsmnf_pair pairs[32];
    jsmnf_init(&loader);
    if (jsmnf_load(&loader, text, tokens, parser.toknext, pairs,
                   sizeof(pairs) / sizeof *pairs)
        <= 0)
        return false;

    jsmnf_pair *f;
    if ((f = jsmnf_find(pairs, text, "url", 3))) {
        const char *url = text + f->v.pos;
        int url_len = (int)f->v.len;

        url_len = snprintf(session->base_url, sizeof(session->base_url),
                           "%.*s%s" DISCORD_GATEWAY_URL_SUFFIX, url_len, url,
                           ('/' == url[url_len - 1]) ? "" : "/");
        ASSERT_NOT_OOB(url_len, sizeof(session->base_url));
    }
    if ((f = jsmnf_find(pairs, text, "shards", 6)))
        session->shards = (int)strtol(text + f->v.pos, NULL, 10);
    if ((f = jsmnf_find(pairs, text, "session_start_limit", 19)))
        discord_session_start_limit_from_jsmnf(f, text, &session->start_limit);
    return true;
}

CCORDcode
discord_gateway_start(struct discord_gateway *gw)
{
    struct ccord_szbuf json = { 0 };

    if (gw->session->retry.attempt == gw->session->retry.limit) {
        logconf_fatal(&gw->conf,
                      "Failed reconnecting to Discord after %d tries",
                      gw->session->retry.limit);

        return CCORD_DISCORD_CONNECTION;
    }

    if (discord_get_gateway_bot(CLIENT(gw, gw), &json) != CCORD_OK
        || !_discord_gateway_session_from_json(gw->session, json.start,
                                               json.size))
    {
        logconf_fatal(&gw->conf, "Couldn't retrieve Gateway Bot information");
        free(json.start);

        return CCORD_DISCORD_BAD_AUTH;
    }
    free(json.start);

    if (!gw->session->start_limit.remaining) {
        logconf_fatal(&gw->conf,
                      "Reach sessions threshold (%d),"
                      "Please wait %d seconds and try again",
                      gw->session->start_limit.total,
                      gw->session->start_limit.reset_after / 1000);

        return CCORD_DISCORD_RATELIMIT;
    }

    if (gw->session->status & DISCORD_SESSION_RESUMABLE
        && *gw->session->resume_url)
    {
        ws_set_url(gw->ws, gw->session->resume_url);
        *gw->session->resume_url = '\0';
    }
    else {
        ws_set_url(gw->ws, gw->session->base_url);
    }

#ifndef CCORD_DEBUG_WEBSOCKETS
    ws_start(gw->ws);
#else
    CURL *ehandle = ws_start(gw->ws);
    curl_easy_setopt(ehandle, CURLOPT_DEBUGFUNCTION, _ws_curl_debug_trace);
    curl_easy_setopt(ehandle, CURLOPT_VERBOSE, 1L);
#endif /* CCORD_DEBUG_WEBSOCKETS */

    return CCORD_OK;
}

bool
discord_gateway_end(struct discord_gateway *gw)
{
    ws_end(gw->ws);

    /* keep only resumable information */
    gw->session->status &= DISCORD_SESSION_RESUMABLE;
    gw->session->is_ready = false;

    if (!gw->session->retry.enable) {
        logconf_warn(&gw->conf, "Discord Gateway Shutdown");

        /* reset for next run */
        gw->session->status = DISCORD_SESSION_OFFLINE;
        gw->session->is_ready = false;
        gw->session->retry.enable = false;
        gw->session->retry.attempt = 0;

        return true;
    }

    ++gw->session->retry.attempt;

    logconf_info(&gw->conf, "Reconnect attempt #%d",
                 gw->session->retry.attempt);

    return false;
}

CCORDcode
discord_gateway_perform(struct discord_gateway *gw)
{
    switch (ws_get_status(gw->ws)) {
    case WS_CONNECTING:
    case WS_CONNECTED:
        if (ws_multi_socket_run(gw->ws, &gw->timer->now)) return CCORD_OK;
    /* fall-through */
    default:
        return CCORD_DISCORD_CONNECTION;
    }
}

void
discord_gateway_shutdown(struct discord_gateway *gw)
{
    const char reason[] = "Client triggered shutdown";

    /* TODO: MT-Unsafe section */
    gw->session->retry.enable = false;
    gw->session->status = DISCORD_SESSION_SHUTDOWN;

    ws_close(gw->ws, WS_CLOSE_REASON_NORMAL, reason, sizeof(reason));
    io_poller_curlm_enable_perform(CLIENT(gw, gw)->io_poller, gw->mhandle);
}

void
discord_gateway_reconnect(struct discord_gateway *gw, bool resume)
{
    const char reason[] = "Client triggered reconnect";
    enum ws_close_reason opcode;

    /* TODO: MT-Unsafe section */
    gw->session->retry.enable = true;
    gw->session->status = DISCORD_SESSION_SHUTDOWN;
    if (resume) {
        gw->session->status |= DISCORD_SESSION_RESUMABLE;
        opcode = (enum ws_close_reason)DISCORD_GATEWAY_CLOSE_REASON_RECONNECT;
    }
    else {
        opcode = WS_CLOSE_REASON_NORMAL;
    }

    ws_close(gw->ws, opcode, reason, sizeof(reason));
    io_poller_curlm_enable_perform(CLIENT(gw, gw)->io_poller, gw->mhandle);
}
