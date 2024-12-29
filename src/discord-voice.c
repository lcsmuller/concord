#include <stdio.h>
#include <string.h>

#include "discord.h"
#include "discord-voice.h"
#include "discord-internal.h"
#include "cog-utils.h"

/**
 * @brief Discord Voice Connection handle, contain information
 *        about its active session.
 *
 * @note struct discord_voice are reused on a guild basis, because there can
 *        be only one active struct discord_voice session per guild.
 * @see discord_voice_join()
 * @see discord_voice_get_vc()
 */
struct discord_voice {
    /** `DISCORD_VOICE` logging module */
    struct logconf conf;
    /** the session guild id @note obtained from discord_voice_join() */
    u64snowflake guild_id;
    /** the session channel id @note obtained from discord_voice_join() */
    u64snowflake channel_id;
    /** @note obtained from on_voice_server_update() */
    /** the session token @note obtained from on_voice_server_update() */
    char token[128];
    /** the new session token after a voice region change @note obtained from
     * on_voice_server_update() */
    char new_token[128];
    /** the new url after a voice region change @note obtained from
     * on_voice_server_update() */
    char new_url[512];
    /** @note obtained from on_voice_state_update()
     * the session id @note obtained from on_voice_state_update() */
    char session_id[128];
    CURLM *mhandle;
    /** the websockets handle that binds to Discord Voice Connections */
    struct websockets *ws;

    /** @brief handle reconnect logic */
    /* reconnect structure */
    struct {
        /** will attempt reconnecting if true */
        bool enable;
        /** current reconnect attempt (resets to 0 when succesful) */
        unsigned char attempt;
        /** max amount of reconnects before giving up */
        unsigned char threshold;
    } reconnect;

    /** will attempt to resume session if connection shutsdown */
    bool is_resumable;
    /** redirect to a different voice server */
    bool is_redirect;
    /** can start sending/receiving additional events to discord */
    bool is_ready;

    /** current iteration JSON string data */
    char *json;
    /** current iteration JSON string data length */
    size_t length;

    /** parse JSON tokens into a `jsmnf_pairs` key/value pairs hashtable */
    struct {
        /** current iteration JSON key/value pairs */
        jsmnf_pair *pairs;
        /** current iteration number of JSON key/value pairs */
        unsigned npairs;
        /** current iteration JSON tokens (fed to `jsmnf_pair`) */
        jsmntok_t *tokens;
        /** current iteration number of JSON tokens */
        unsigned ntokens;
    } parse;

    /** voice payload structure */
    struct {
        /** field 'op' */
        enum discord_voice_opcodes opcode;
        /** field 'd' */
        jsmnf_pair *data;
    } payload;

    /** heartbeat structure */
    struct {
        /** fixed interval between heartbeats */
        u64unix_ms interval_ms;
        /** start pulse timestamp in milliseconds */
        u64unix_ms tstamp;
    } hbeat;

    /** latency between client and websockets server, calculated by the
     * interval between HEARTBEAT and HEARTBEAT_ACK */
    int ping_ms;

    /** if true shutdown websockets connection as soon as possible */
    bool shutdown;

    struct {
        int ssrc;
        int server_port;
        char server_ip[256];
        char digest[256];
        char unique_key[128];
        int audio_udp_pid;
        uintmax_t start_time;
    } udp_service;

    struct discord_voice_evcallbacks *p_voice_cbs;

    /**
     * @brief Interval to divide the received packets
     *
     * 0 store in one file
     * n store packets received every n minutes in a new file
     */
    int recv_interval;

    /** pointer to client this struct is part of */
    struct discord *p_client;
};

/* return enumerator as string in case of a match */
#define CASE_RETURN_STR(code)                                                 \
    case code:                                                                \
        return #code

/* TODO: use a per-client lock instead */
static pthread_mutex_t client_lock = PTHREAD_MUTEX_INITIALIZER;

static const char *
opcode_print(enum discord_voice_opcodes opcode)
{
    switch (opcode) {
        CASE_RETURN_STR(DISCORD_VOICE_IDENTIFY);
        CASE_RETURN_STR(DISCORD_VOICE_SELECT_PROTOCOL);
        CASE_RETURN_STR(DISCORD_VOICE_READY);
        CASE_RETURN_STR(DISCORD_VOICE_HEARTBEAT);
        CASE_RETURN_STR(DISCORD_VOICE_SESSION_DESCRIPTION);
        CASE_RETURN_STR(DISCORD_VOICE_SPEAKING);
        CASE_RETURN_STR(DISCORD_VOICE_RESUME);
        CASE_RETURN_STR(DISCORD_VOICE_HELLO);
        CASE_RETURN_STR(DISCORD_VOICE_RESUMED);
        CASE_RETURN_STR(DISCORD_VOICE_CLIENT_DISCONNECT);
        CASE_RETURN_STR(DISCORD_VOICE_CODEC);
    default:
        return "INVALID_GATEWAY_OPCODE";
    }
}

static const char *
close_opcode_print(enum discord_voice_close_opcodes opcode)
{
    switch (opcode) {
        CASE_RETURN_STR(DISCORD_VOICE_CLOSE_REASON_UNKNOWN_OPCODE);
        CASE_RETURN_STR(DISCORD_VOICE_CLOSE_REASON_DECODE_ERROR);
        CASE_RETURN_STR(DISCORD_VOICE_CLOSE_REASON_NOT_AUTHENTICATED);
        CASE_RETURN_STR(DISCORD_VOICE_CLOSE_REASON_ALREADY_AUTHENTICATED);
        CASE_RETURN_STR(DISCORD_VOICE_CLOSE_REASON_AUTHENTICATION_FAILED);
        CASE_RETURN_STR(DISCORD_VOICE_CLOSE_REASON_INVALID_SESSION);
        CASE_RETURN_STR(DISCORD_VOICE_CLOSE_REASON_SESSION_TIMED_OUT);
        CASE_RETURN_STR(DISCORD_VOICE_CLOSE_REASON_SERVER_NOT_FOUND);
        CASE_RETURN_STR(DISCORD_VOICE_CLOSE_REASON_UNKNOWN_PROTOCOL);
        CASE_RETURN_STR(DISCORD_VOICE_CLOSE_REASON_DISCONNECTED);
        CASE_RETURN_STR(DISCORD_VOICE_CLOSE_REASON_SERVER_CRASH);
        CASE_RETURN_STR(DISCORD_VOICE_CLOSE_REASON_UNKNOWN_ENCRYPTION_MODE);
    default: {
        const char *str;

        str = ws_close_opcode_print((enum ws_close_reason)opcode);
        if (str) return str;

        log_warn("Unknown WebSockets close opcode (code: %d)", opcode);
    }
        return "UNKNOWN_WEBSOCKETS_CLOSE_OPCODE";
    }
}

static void
send_resume(struct discord_voice *vc)
{
    char buf[1024];
    jsonb b;

    vc->is_resumable = false; /* reset */

    jsonb_init(&b);
    jsonb_object(&b, buf, sizeof(buf));
    {
        jsonb_key(&b, buf, sizeof(buf), "op", 2);
        jsonb_number(&b, buf, sizeof(buf), 7);
        jsonb_key(&b, buf, sizeof(buf), "d", 1);
        jsonb_object(&b, buf, sizeof(buf));
        {
            char tok[32];
            int toklen = snprintf(tok, sizeof(tok), "%" PRIu64, vc->guild_id);

            if (toklen > 0) {
                jsonb_key(&b, buf, sizeof(buf), "server_id", 9);
                jsonb_token(&b, buf, sizeof(buf), tok, (size_t)toklen);
            }
            jsonb_key(&b, buf, sizeof(buf), "session_id", 10);
            jsonb_string(&b, buf, sizeof(buf), vc->session_id,
                         strlen(vc->session_id));
            jsonb_key(&b, buf, sizeof(buf), "token", 5);
            jsonb_string(&b, buf, sizeof(buf), vc->token, strlen(vc->token));
            jsonb_object_pop(&b, buf, sizeof(buf));
        }
        jsonb_object_pop(&b, buf, sizeof(buf));
    }

    logconf_info(
        &vc->conf,
        ANSICOLOR("SEND", ANSI_FG_BRIGHT_GREEN) " VOICE_RESUME (%d bytes)",
        b.pos);

    ws_send_text(vc->ws, NULL, buf, b.pos);
}

static void
send_identify(struct discord_voice *vc)
{
    const struct discord_user *self = discord_get_self(vc->p_client);
    char buf[1024];
    jsonb b;

    jsonb_init(&b);
    jsonb_object(&b, buf, sizeof(buf));
    {
        jsonb_key(&b, buf, sizeof(buf), "op", 2);
        jsonb_number(&b, buf, sizeof(buf), 0);
        jsonb_key(&b, buf, sizeof(buf), "d", 1);
        jsonb_object(&b, buf, sizeof(buf));
        {
            char tok[32];
            int toklen = snprintf(tok, sizeof(tok), "%" PRIu64, vc->guild_id);

            if (toklen > 0) {
                jsonb_key(&b, buf, sizeof(buf), "server_id", 9);
                jsonb_token(&b, buf, sizeof(buf), tok, (size_t)toklen);
            }
            toklen = snprintf(tok, sizeof(tok), "%" PRIu64, self->id);
            if (toklen > 0) {
                jsonb_key(&b, buf, sizeof(buf), "user_id", 7);
                jsonb_token(&b, buf, sizeof(buf), tok, (size_t)toklen);
            }
            jsonb_key(&b, buf, sizeof(buf), "session_id", 10);
            jsonb_string(&b, buf, sizeof(buf), vc->session_id,
                         strlen(vc->session_id));
            jsonb_key(&b, buf, sizeof(buf), "token", 5);
            jsonb_string(&b, buf, sizeof(buf), vc->token, strlen(vc->token));
            jsonb_object_pop(&b, buf, sizeof(buf));
        }
        jsonb_object_pop(&b, buf, sizeof(buf));
    }

    logconf_info(
        &vc->conf,
        ANSICOLOR("SEND", ANSI_FG_BRIGHT_GREEN) " VOICE_IDENTIFY (%d bytes)",
        b.pos);

    ws_send_text(vc->ws, NULL, buf, b.pos);
}

static void
on_hello(struct discord_voice *vc)
{
    float hbeat_interval = 0.0f;
    jsmnf_pair *f;

    vc->hbeat.tstamp = cog_timestamp_ms();
    if ((f = jsmnf_find(vc->payload.data, vc->json, "heartbeat_interval", 18)))
        hbeat_interval = strtof(vc->json + f->v.pos, NULL);

    vc->hbeat.interval_ms =
        (hbeat_interval < 5000.0f) ? (u64unix_ms)hbeat_interval : 5000;

    if (vc->is_resumable)
        send_resume(vc);
    else
        send_identify(vc);
}

static void
on_ready(struct discord_voice *vc)
{
    struct discord *client = vc->p_client;

    logconf_info(&vc->conf, "Succesfully started a Discord Voice session!");
    vc->is_ready = true;
    vc->reconnect.attempt = 0;

    if (client->voice_cbs.on_ready) client->voice_cbs.on_ready(vc);
}

static void
on_session_description(struct discord_voice *vc)
{
    struct discord *client = vc->p_client;

    if (client->voice_cbs.on_session_descriptor)
        client->voice_cbs.on_session_descriptor(vc);
}

static void
on_speaking(struct discord_voice *vc)
{
    struct discord *client = vc->p_client;

    int speaking = 0, delay = 0, ssrc = 0;
    u64snowflake user_id = 0;
    jsmnf_pair *f;

    if (!client->voice_cbs.on_speaking) return;

    if ((f = jsmnf_find(vc->payload.data, vc->json, "user_id", 7)))
        sscanf(vc->json + f->v.pos, "%" SCNu64, &user_id);
    if ((f = jsmnf_find(vc->payload.data, vc->json, "speaking", 8)))
        speaking = (int)strtol(vc->json + f->v.pos, NULL, 10);
    if ((f = jsmnf_find(vc->payload.data, vc->json, "delay", 5)))
        delay = (int)strtol(vc->json + f->v.pos, NULL, 10);
    if ((f = jsmnf_find(vc->payload.data, vc->json, "ssrc", 4)))
        ssrc = (int)strtol(vc->json + f->v.pos, NULL, 10);

    client->voice_cbs.on_speaking(client, vc, user_id, speaking, delay, ssrc);
}

static void
on_resumed(struct discord_voice *vc)
{
    vc->is_ready = true;
    vc->reconnect.attempt = 0;
    logconf_info(&vc->conf, "Successfully resumed a Discord Voice session!");
}

static void
on_client_disconnect(struct discord_voice *vc)
{
    struct discord *client = vc->p_client;
    u64snowflake user_id = 0;
    jsmnf_pair *f;

    if (!client->voice_cbs.on_client_disconnect) return;

    if ((f = jsmnf_find(vc->payload.data, vc->json, "user_id", 7)))
        sscanf(vc->json + f->v.pos, "%" SCNu64, &user_id);

    client->voice_cbs.on_client_disconnect(client, vc, user_id);
}

static void
on_codec(struct discord_voice *vc)
{
    struct discord *client = vc->p_client;
    char audio_codec[64] = "", video_codec[64] = "";
    jsmnf_pair *f;

    if (!client->voice_cbs.on_codec) return;

    if ((f = jsmnf_find(vc->payload.data, vc->json, "audio_codec", 11)))
        snprintf(audio_codec, sizeof(audio_codec), "%.*s", (int)f->v.len,
                 vc->json + f->v.pos);
    if ((f = jsmnf_find(vc->payload.data, vc->json, "video_codec", 11)))
        snprintf(video_codec, sizeof(video_codec), "%.*s", (int)f->v.len,
                 vc->json + f->v.pos);

    client->voice_cbs.on_codec(client, vc, audio_codec, video_codec);
}

static void
on_heartbeat_ack(struct discord_voice *vc)
{
    /* get request / response interval in milliseconds */
    vc->ping_ms = (int)(cog_timestamp_ms() - vc->hbeat.tstamp);
    logconf_trace(&vc->conf, "PING: %d ms", vc->ping_ms);
}

static void
on_connect_cb(void *p_vc,
              struct websockets *ws,
              struct ws_info *info,
              const char *ws_protocols)
{
    struct discord_voice *vc = p_vc;
    (void)ws;
    (void)info;

    logconf_info(&vc->conf, "Connected, WS-Protocols: '%s'", ws_protocols);
}

static void
on_close_cb(void *p_vc,
            struct websockets *ws,
            struct ws_info *info,
            enum ws_close_reason wscode,
            const char *reason,
            size_t len)
{
    struct discord_voice *vc = p_vc;
    enum discord_voice_close_opcodes opcode =
        (enum discord_voice_close_opcodes)wscode;
    (void)ws;
    (void)info;

    logconf_warn(
        &vc->conf,
        ANSICOLOR("CLOSE %s", ANSI_FG_RED) " (code: %4d, %zu bytes): '%.*s'",
        close_opcode_print(opcode), opcode, len, (int)len, reason);

    /* user-triggered shutdown */
    if (vc->shutdown) return;

    switch (opcode) {
    case DISCORD_VOICE_CLOSE_REASON_SERVER_CRASH:
        vc->is_resumable = true;
        vc->reconnect.enable = true;
        break;
    case DISCORD_VOICE_CLOSE_REASON_UNKNOWN_OPCODE:
    case DISCORD_VOICE_CLOSE_REASON_DECODE_ERROR:
    case DISCORD_VOICE_CLOSE_REASON_NOT_AUTHENTICATED:
    case DISCORD_VOICE_CLOSE_REASON_AUTHENTICATION_FAILED:
    case DISCORD_VOICE_CLOSE_REASON_ALREADY_AUTHENTICATED:
    case DISCORD_VOICE_CLOSE_REASON_SERVER_NOT_FOUND:
    case DISCORD_VOICE_CLOSE_REASON_UNKNOWN_PROTOCOL:
    case DISCORD_VOICE_CLOSE_REASON_UNKNOWN_ENCRYPTION_MODE:
        vc->is_resumable = false;
        vc->reconnect.enable = false;
        break;
    case DISCORD_VOICE_CLOSE_REASON_DISCONNECTED:
        vc->is_resumable = false;
        vc->reconnect.enable = true;
        break;
    default: /*websocket/clouflare opcodes */
        if (WS_CLOSE_REASON_NORMAL == (enum ws_close_reason)opcode) {
            vc->is_resumable = true;
            vc->reconnect.enable = true;
        }
        else {
            vc->is_resumable = false;
            vc->reconnect.enable = false;
        }
        break;
    case DISCORD_VOICE_CLOSE_REASON_SESSION_TIMED_OUT:
    case DISCORD_VOICE_CLOSE_REASON_INVALID_SESSION:
        vc->is_resumable = false;
        vc->reconnect.enable = true;
        break;
    }
}

static void
on_text_cb(void *p_vc,
           struct websockets *ws,
           struct ws_info *info,
           const char *text,
           size_t len)
{
    (void)ws;
    (void)info;
    struct discord_voice *vc = p_vc;
    jsmn_parser parser;

    vc->json = (char *)text;
    vc->length = len;

    jsmn_init(&parser);
    if (0 < jsmn_parse_auto(&parser, text, len, &vc->parse.tokens,
                            &vc->parse.ntokens))
    {
        jsmnf_loader loader;

        jsmnf_init(&loader);
        if (0 < jsmnf_load_auto(&loader, text, vc->parse.tokens,
                                parser.toknext, &vc->parse.pairs,
                                &vc->parse.npairs))
        {
            jsmnf_pair *f;

            if ((f = jsmnf_find(vc->parse.pairs, vc->json, "op", 2)))
                vc->payload.opcode =
                    (int)strtol(vc->json + f->v.pos, NULL, 10);
            vc->payload.data = jsmnf_find(vc->parse.pairs, vc->json, "d", 1);
        }
    }

    logconf_trace(
        &vc->conf,
        ANSICOLOR("RCV", ANSI_FG_BRIGHT_YELLOW) " VOICE_%s (%zu bytes)",
        opcode_print(vc->payload.opcode), len);

    switch (vc->payload.opcode) {
    case DISCORD_VOICE_READY:
        on_ready(vc);
        break;
    case DISCORD_VOICE_SESSION_DESCRIPTION:
        on_session_description(vc);
        break;
    case DISCORD_VOICE_SPEAKING:
        on_speaking(vc);
        break;
    case DISCORD_VOICE_HEARTBEAT_ACK:
        on_heartbeat_ack(vc);
        break;
    case DISCORD_VOICE_HELLO:
        on_hello(vc);
        break;
    case DISCORD_VOICE_RESUMED:
        on_resumed(vc);
        break;
    case DISCORD_VOICE_CLIENT_DISCONNECT:
        on_client_disconnect(vc);
        break;
    case DISCORD_VOICE_CODEC:
        on_codec(vc);
        break;
    default:
        logconf_error(&vc->conf, "Not yet implemented Voice Event(code: %d)",
                      vc->payload.opcode);
        break;
    }
}

/* send heartbeat pulse to websockets server in order
 *  to maintain connection alive */
static void
send_heartbeat(struct discord_voice *vc)
{
    char buf[64];
    jsonb b;

    jsonb_init(&b);
    jsonb_object(&b, buf, sizeof(buf));
    {
        jsonb_key(&b, buf, sizeof(buf), "op", sizeof("op") - 1);
        jsonb_number(&b, buf, sizeof(buf), 3);
        jsonb_key(&b, buf, sizeof(buf), "d", sizeof("d") - 1);
        jsonb_number(&b, buf, sizeof(buf), (double)vc->hbeat.interval_ms);
        jsonb_object_pop(&b, buf, sizeof(buf));
    }

    logconf_info(
        &vc->conf,
        ANSICOLOR("SEND", ANSI_FG_BRIGHT_GREEN) " VOICE_HEARTBEAT (%d bytes)",
        b.pos);

    ws_send_text(vc->ws, NULL, buf, b.pos);
}

static void
reset_vc(struct discord_voice *vc)
{
    vc->reconnect.attempt = 0;
    vc->shutdown = false;
}

static void
_discord_voice_init(struct discord_voice *new_vc,
                    struct discord *client,
                    u64snowflake guild_id,
                    u64snowflake channel_id)
{
    new_vc->p_client = client;
    new_vc->guild_id = guild_id;
    new_vc->channel_id = channel_id;

    if (NULL == new_vc->ws) {
        struct ws_callbacks cbs = {
            .data = new_vc,
            .on_connect = &on_connect_cb,
            .on_text = &on_text_cb,
            .on_close = &on_close_cb,
        };

        struct ws_attr attr = {
            .conf = &client->conf,
        };

        new_vc->mhandle = curl_multi_init();
        new_vc->ws = ws_init(&cbs, new_vc->mhandle, &attr);
        logconf_branch(&new_vc->conf, &client->conf, "DISCORD_VOICE");

        new_vc->reconnect.threshold = 5; /* TODO: shouldn't be a hard limit */
        new_vc->reconnect.enable = true;
    }

    reset_vc(new_vc);
}

CCORDcode
discord_send_speaking(struct discord_voice *vc, u64bitmask flags, int delay)
{
    char buf[128];
    jsonb b;

    if (ws_get_status(vc->ws) != WS_CONNECTED) {
        logconf_error(&vc->conf,
                      "Action requires an active connection to Discord");
        return CCORD_DISCORD_CONNECTION;
    }

    jsonb_init(&b);
    jsonb_object(&b, buf, sizeof(buf));
    {
        jsonb_key(&b, buf, sizeof(buf), "op", sizeof("op") - 1);
        jsonb_number(&b, buf, sizeof(buf), 7);
        jsonb_key(&b, buf, sizeof(buf), "d", sizeof("d") - 1);
        jsonb_object(&b, buf, sizeof(buf));
        {
            jsonb_key(&b, buf, sizeof(buf), "speaking",
                      sizeof("speaking") - 1);
            jsonb_number(&b, buf, sizeof(buf), (int)flags);
            jsonb_key(&b, buf, sizeof(buf), "delay", sizeof("delay") - 1);
            jsonb_number(&b, buf, sizeof(buf), delay);
            jsonb_key(&b, buf, sizeof(buf), "ssrc", sizeof("ssrc") - 1);
            jsonb_number(&b, buf, sizeof(buf), vc->udp_service.ssrc);
            jsonb_object_pop(&b, buf, sizeof(buf));
        }
        jsonb_object_pop(&b, buf, sizeof(buf));
    }

    logconf_info(
        &vc->conf,
        ANSICOLOR("SEND", ANSI_FG_BRIGHT_GREEN) " VOICE_SPEAKING (%d bytes)",
        b.pos);

    return ws_send_text(vc->ws, NULL, buf, b.pos) ? CCORD_OK
                                                  : CCORD_CURL_NO_RESPONSE;
}

static void
recycle_active_vc(struct discord_voice *vc,
                  u64snowflake guild_id,
                  u64snowflake channel_id)
{
    if (ws_is_alive(vc->ws)) {
        discord_voice_shutdown(vc);
    }
    vc->channel_id = channel_id;
    vc->guild_id = guild_id;
    vc->shutdown = false;
}

enum discord_voice_status
discord_voice_join(struct discord *client,
                   u64snowflake guild_id,
                   u64snowflake vchannel_id,
                   bool self_mute,
                   bool self_deaf)
{
    struct discord_update_voice_state state = { .guild_id = guild_id,
                                                .channel_id = vchannel_id,
                                                .self_mute = self_mute,
                                                .self_deaf = self_deaf };
    bool found_a_running_vcs = false;
    struct discord_voice *vc = NULL;

    if (!ws_is_functional(client->gw.ws)) return DISCORD_VOICE_ERROR;

    pthread_mutex_lock(&client_lock);
    for (int i = 0; i < DISCORD_MAX_VCS; ++i) {
        if (0 == client->vcs[i].guild_id) {
            vc = client->vcs + i;
            _discord_voice_init(vc, client, guild_id, vchannel_id);
            break;
        }
        if (guild_id == client->vcs[i].guild_id) {
            if (vchannel_id == client->vcs[i].channel_id) {
                found_a_running_vcs = true;
            }
            vc = client->vcs + i;
            break;
        }
    }
    pthread_mutex_unlock(&client_lock);

    if (!vc) {
        logconf_error(&client->conf,
                      "All VC are busy, cannot send VOICE_STATE_UPDATE");
        /* run out of vcs connections, report error to users */
        return DISCORD_VOICE_EXHAUST_CAPACITY;
    }
    if (found_a_running_vcs) {
        return DISCORD_VOICE_ALREADY_JOINED;
    }

    recycle_active_vc(vc, guild_id, vchannel_id);
    discord_gateway_send_update_voice_state(&client->gw, &state);

    return DISCORD_VOICE_JOINED;
}

/*
 * The normal situations that we expect:
 * 1. join a vc
 * 2. leave a vc
 * 3. Discord just kicks the bot out of a vc.
 */
void
_discord_on_voice_state_update(struct discord *client,
                               struct discord_voice_state *event)
{
    struct discord_voice *vc = NULL;

    pthread_mutex_lock(&client_lock);
    for (int i = 0; i < DISCORD_MAX_VCS; ++i) {
        if (event->guild_id == client->vcs[i].guild_id) {
            vc = client->vcs + i;
            if (event->channel_id) {
                int len = snprintf(vc->session_id, sizeof(vc->session_id),
                                   "%s", event->session_id);
                ASSERT_NOT_OOB(len, sizeof(vc->session_id));

                logconf_info(&vc->conf,
                             "Starting a new voice session (id: " ANSICOLOR(
                                 "%s", ANSI_FG_YELLOW) ")",
                             vc->session_id);
            }
            break;
        }
    }
    pthread_mutex_unlock(&client_lock);

    if (!vc) {
        if (event->channel_id) {
            logconf_fatal(
                &client->conf,
                "This should not happen, cannot find a discord_voice object");
            /* report this */
        }
        return;
    }

    if (event->channel_id == 0) {
        logconf_info(&vc->conf, ANSICOLOR("Bot is leaving the current vc",
                                          ANSI_BG_BRIGHT_BLUE));
        if (vc->ws && ws_is_alive(vc->ws))
            logconf_warn(&vc->conf, "Voice ws is still alive");
        return;
    }
}

static void
event_loop(struct discord_voice *vc)
{
    struct discord *client = vc->p_client;
    uint64_t tstamp;

    /* everything goes well, ws event_loop to serve */
    /* the ws server side events */
    ws_start(vc->ws);
    while (1) {
        /* break on severed connection */
        if (!ws_easy_run(vc->ws, 5, &tstamp)) break;

        /* wait until connection shutdown */
        if (vc->shutdown) continue;
        /* wait until client is ready */
        if (!vc->is_ready) continue;

        /* check if timespan since first pulse is greater than
         * minimum heartbeat interval required*/
        if (vc->hbeat.interval_ms < (ws_timestamp(vc->ws) - vc->hbeat.tstamp))
        {
            send_heartbeat(vc);
            vc->hbeat.tstamp =
                ws_timestamp(vc->ws); /*update heartbeat timestamp */
        }
        if (client->voice_cbs.on_idle) client->voice_cbs.on_idle(client, vc);
    }
    ws_end(vc->ws);

    vc->shutdown = false;
    vc->is_ready = false;
}

static void *
start_voice_ws_thread(void *p_vc)
{
    struct discord_voice *vc = p_vc;

    /* handle ws reconnect/resume/redirect logic */
    while (vc->reconnect.attempt < vc->reconnect.threshold) {
        event_loop(vc);

        if (vc->is_redirect) {
            memcpy(vc->token, vc->new_token, sizeof(vc->token));
            ws_set_url(vc->ws, vc->new_url);
            vc->is_redirect = false;
            vc->reconnect.attempt = 0;
            vc->reconnect.enable = true;
            vc->is_resumable = false;
            continue;
        }

        if (!vc->reconnect.enable) {
            logconf_warn(&vc->conf, "Discord VC shutdown is complete");
            goto _end;
        }

        ++vc->reconnect.attempt;
        logconf_info(&vc->conf, "Reconnect attempt #%d",
                     vc->reconnect.attempt);
    }

    if (!vc->shutdown)
        logconf_error(&vc->conf,
                      "Could not reconnect to Discord Voice after %d tries",
                      vc->reconnect.threshold);

    /* exit from the event loop; */
_end:
    reset_vc(vc);
    vc->guild_id = 0; /* put this back to the pool */
    return NULL;
}

/*
 * 1. join a vc -> create a new ws connection
 * 2. change voice region -> redirect an existing ws connection
 */
void
_discord_on_voice_server_update(struct discord *client,
                                struct discord_voice_server_update *event)
{
    struct discord_voice *vc = NULL;
    int len;

    pthread_mutex_lock(&client_lock);
    for (int i = 0; i < DISCORD_MAX_VCS; ++i) {
        if (event->guild_id == client->vcs[i].guild_id) {
            vc = client->vcs + i;
            break;
        }
    }
    pthread_mutex_unlock(&client_lock);

    if (!vc) {
        logconf_fatal(&client->conf, "Couldn't match voice server to client");
        return;
    }

    len = snprintf(vc->new_token, sizeof(vc->new_token), "%s", event->token);
    ASSERT_NOT_OOB(len, sizeof(vc->new_token));

    len = snprintf(vc->new_url, sizeof(vc->new_url),
                   "wss://%s" DISCORD_VCS_URL_SUFFIX, event->endpoint);
    ASSERT_NOT_OOB(len, sizeof(vc->new_url));

    /* TODO: replace with the more reliable thread alive check */
    if (ws_is_alive(vc->ws)) {
        /* exits the current event_loop to redirect */
        const char reason[] = "Attempt to redirect";

        vc->is_redirect = true;
        ws_close(vc->ws, WS_CLOSE_REASON_NORMAL, reason, sizeof(reason));
    }
    else {
        pthread_t tid;

        memcpy(vc->token, vc->new_token, sizeof(vc->new_token));
        ws_set_url(vc->ws, vc->new_url);

        /** TODO: replace with a threadpool */
        if (pthread_create(&tid, NULL, &start_voice_ws_thread, vc))
            ERR("Couldn't create thread");
        if (pthread_detach(tid)) ERR("Couldn't detach thread");
    }
}

void
discord_voice_connections_init(struct discord *client)
{
    client->vcs = malloc(DISCORD_MAX_VCS * sizeof *client->vcs);
    for (int i = 0; i < DISCORD_MAX_VCS; ++i)
        client->vcs[i].p_voice_cbs = &client->voice_cbs;
}

static void
_discord_voice_cleanup(struct discord_voice *vc)
{
    if (vc->mhandle) curl_multi_cleanup(vc->mhandle);
    if (vc->ws) ws_cleanup(vc->ws);
    if (vc->parse.pairs) free(vc->parse.pairs);
    if (vc->parse.tokens) free(vc->parse.tokens);
}

void
discord_voice_connections_cleanup(struct discord *client)
{
    for (int i = 0; i < DISCORD_MAX_VCS; ++i)
        _discord_voice_cleanup(&client->vcs[i]);
    free(client->vcs);
}

void
discord_voice_shutdown(struct discord_voice *vc)
{
    struct discord_update_voice_state state = { .guild_id = vc->guild_id };
    const char reason[] = "Client triggered voice shutdown";

    vc->reconnect.enable = false;
    vc->shutdown = true;
    vc->is_resumable = false;

    discord_gateway_send_update_voice_state(&vc->p_client->gw, &state);
    ws_close(vc->ws, WS_CLOSE_REASON_NORMAL, reason, sizeof(reason));
}

void
discord_voice_reconnect(struct discord_voice *vc, bool resume)
{
    const char reason[] = "Client triggered voice reconnect";
    enum ws_close_reason opcode;

    vc->reconnect.enable = true;
    vc->shutdown = true;
    vc->is_resumable = resume;
    opcode =
        vc->is_resumable ? WS_CLOSE_REASON_NO_REASON : WS_CLOSE_REASON_NORMAL;

    ws_close(vc->ws, opcode, reason, sizeof(reason));
}

bool
discord_voice_is_alive(struct discord_voice *vc)
{
    return vc->guild_id && ws_is_alive(vc->ws);
}

void
discord_set_voice_cbs(struct discord *client,
                      struct discord_voice_evcallbacks *callbacks)
{
    if (callbacks->on_speaking)
        client->voice_cbs.on_speaking = callbacks->on_speaking;
    if (callbacks->on_codec) client->voice_cbs.on_codec = callbacks->on_codec;
    if (callbacks->on_session_descriptor)
        client->voice_cbs.on_session_descriptor =
            callbacks->on_session_descriptor;
    if (callbacks->on_client_disconnect)
        client->voice_cbs.on_client_disconnect =
            callbacks->on_client_disconnect;
    if (callbacks->on_ready) client->voice_cbs.on_ready = callbacks->on_ready;
    if (callbacks->on_idle) client->voice_cbs.on_idle = callbacks->on_idle;
    if (callbacks->on_udp_server_connected)
        client->voice_cbs.on_udp_server_connected =
            callbacks->on_udp_server_connected;
    discord_add_intents(client, DISCORD_GATEWAY_GUILD_VOICE_STATES);
}
