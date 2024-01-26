#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "discord.h"
#include "discord-internal.h"

#define INIT(type)                                                            \
    {                                                                         \
        sizeof(struct type),                                                  \
            (size_t(*)(jsmnf_pair *, const char *, void *))type##_from_jsmnf, \
            (void (*)(void *))type##_cleanup                                  \
    }

/** @brief Information for deserializing a Discord event */
static const struct {
    /** size of event's datatype */
    size_t size;
    /** event's payload deserializer */
    size_t (*from_jsmnf)(jsmnf_pair *, const char *, void *);
    /** event's cleanup */
    void (*cleanup)(void *);
} dispatch[] = {
    [DISCORD_EV_READY] = INIT(discord_ready),
    [DISCORD_EV_APPLICATION_COMMAND_PERMISSIONS_UPDATE] =
        INIT(discord_application_command_permissions),
    [DISCORD_EV_AUTO_MODERATION_RULE_CREATE] =
        INIT(discord_auto_moderation_rule),
    [DISCORD_EV_AUTO_MODERATION_RULE_UPDATE] =
        INIT(discord_auto_moderation_rule),
    [DISCORD_EV_AUTO_MODERATION_RULE_DELETE] =
        INIT(discord_auto_moderation_rule),
    [DISCORD_EV_AUTO_MODERATION_ACTION_EXECUTION] =
        INIT(discord_auto_moderation_action_execution),
    [DISCORD_EV_CHANNEL_CREATE] = INIT(discord_channel),
    [DISCORD_EV_CHANNEL_UPDATE] = INIT(discord_channel),
    [DISCORD_EV_CHANNEL_DELETE] = INIT(discord_channel),
    [DISCORD_EV_CHANNEL_PINS_UPDATE] = INIT(discord_channel_pins_update),
    [DISCORD_EV_THREAD_CREATE] = INIT(discord_channel),
    [DISCORD_EV_THREAD_UPDATE] = INIT(discord_channel),
    [DISCORD_EV_THREAD_DELETE] = INIT(discord_channel),
    [DISCORD_EV_THREAD_LIST_SYNC] = INIT(discord_thread_list_sync),
    [DISCORD_EV_THREAD_MEMBER_UPDATE] = INIT(discord_thread_member),
    [DISCORD_EV_THREAD_MEMBERS_UPDATE] = INIT(discord_thread_members_update),
    [DISCORD_EV_GUILD_CREATE] = INIT(discord_guild),
    [DISCORD_EV_GUILD_UPDATE] = INIT(discord_guild),
    [DISCORD_EV_GUILD_DELETE] = INIT(discord_guild),
    [DISCORD_EV_GUILD_AUDIT_LOG_ENTRY_CREATE] =
        INIT(discord_audit_log_entry),
    [DISCORD_EV_GUILD_BAN_ADD] = INIT(discord_guild_ban_add),
    [DISCORD_EV_GUILD_BAN_REMOVE] = INIT(discord_guild_ban_remove),
    [DISCORD_EV_GUILD_EMOJIS_UPDATE] = INIT(discord_guild_emojis_update),
    [DISCORD_EV_GUILD_STICKERS_UPDATE] = INIT(discord_guild_stickers_update),
    [DISCORD_EV_GUILD_INTEGRATIONS_UPDATE] =
        INIT(discord_guild_integrations_update),
    [DISCORD_EV_GUILD_MEMBER_ADD] = INIT(discord_guild_member),
    [DISCORD_EV_GUILD_MEMBER_UPDATE] = INIT(discord_guild_member_update),
    [DISCORD_EV_GUILD_MEMBER_REMOVE] = INIT(discord_guild_member_remove),
    [DISCORD_EV_GUILD_MEMBERS_CHUNK] = INIT(discord_guild_members_chunk),
    [DISCORD_EV_GUILD_ROLE_CREATE] = INIT(discord_guild_role_create),
    [DISCORD_EV_GUILD_ROLE_UPDATE] = INIT(discord_guild_role_update),
    [DISCORD_EV_GUILD_ROLE_DELETE] = INIT(discord_guild_role_delete),
    [DISCORD_EV_GUILD_SCHEDULED_EVENT_CREATE] =
        INIT(discord_guild_scheduled_event),
    [DISCORD_EV_GUILD_SCHEDULED_EVENT_UPDATE] =
        INIT(discord_guild_scheduled_event),
    [DISCORD_EV_GUILD_SCHEDULED_EVENT_DELETE] =
        INIT(discord_guild_scheduled_event),
    [DISCORD_EV_GUILD_SCHEDULED_EVENT_USER_ADD] =
        INIT(discord_guild_scheduled_event_user_add),
    [DISCORD_EV_GUILD_SCHEDULED_EVENT_USER_REMOVE] =
        INIT(discord_guild_scheduled_event_user_remove),
    [DISCORD_EV_INTEGRATION_CREATE] = INIT(discord_integration),
    [DISCORD_EV_INTEGRATION_UPDATE] = INIT(discord_integration),
    [DISCORD_EV_INTEGRATION_DELETE] = INIT(discord_integration_delete),
    [DISCORD_EV_INTERACTION_CREATE] = INIT(discord_interaction),
    [DISCORD_EV_INVITE_CREATE] = INIT(discord_invite_create),
    [DISCORD_EV_INVITE_DELETE] = INIT(discord_invite_delete),
    [DISCORD_EV_MESSAGE_CREATE] = INIT(discord_message),
    [DISCORD_EV_MESSAGE_UPDATE] = INIT(discord_message),
    [DISCORD_EV_MESSAGE_DELETE] = INIT(discord_message_delete),
    [DISCORD_EV_MESSAGE_DELETE_BULK] = INIT(discord_message_delete_bulk),
    [DISCORD_EV_MESSAGE_REACTION_ADD] = INIT(discord_message_reaction_add),
    [DISCORD_EV_MESSAGE_REACTION_REMOVE] =
        INIT(discord_message_reaction_remove),
    [DISCORD_EV_MESSAGE_REACTION_REMOVE_ALL] =
        INIT(discord_message_reaction_remove_all),
    [DISCORD_EV_MESSAGE_REACTION_REMOVE_EMOJI] =
        INIT(discord_message_reaction_remove_emoji),
    [DISCORD_EV_PRESENCE_UPDATE] = INIT(discord_presence_update),
    [DISCORD_EV_STAGE_INSTANCE_CREATE] = INIT(discord_stage_instance),
    [DISCORD_EV_STAGE_INSTANCE_UPDATE] = INIT(discord_stage_instance),
    [DISCORD_EV_STAGE_INSTANCE_DELETE] = INIT(discord_stage_instance),
    [DISCORD_EV_TYPING_START] = INIT(discord_typing_start),
    [DISCORD_EV_USER_UPDATE] = INIT(discord_user),
    [DISCORD_EV_VOICE_STATE_UPDATE] = INIT(discord_voice_state),
    [DISCORD_EV_VOICE_SERVER_UPDATE] = INIT(discord_voice_server_update),
    [DISCORD_EV_WEBHOOKS_UPDATE] = INIT(discord_webhooks_update),
};

void
discord_gateway_dispatch(struct discord_gateway *gw)
{
    const enum discord_gateway_events event = gw->payload.event;
    struct discord *client = CLIENT(gw, gw);

    switch (event) {
    case DISCORD_EV_MESSAGE_CREATE:
        if (discord_message_commands_try_perform(&client->commands,
                                                 &gw->payload)) {
            return;
        }
    /* fall-through */
    default:
        if (gw->cbs[0][event] || gw->cbs[1][event]) {
            void *event_data = calloc(1, dispatch[event].size);

            dispatch[event].from_jsmnf(gw->payload.data,
                                       gw->payload.json.start, event_data);

            if (CCORD_RESOURCE_UNAVAILABLE
                == discord_refcounter_incr(&client->refcounter, event_data))
            {
                discord_refcounter_add_internal(&client->refcounter,
                                                event_data,
                                                dispatch[event].cleanup, true);
            }
            if (gw->cbs[0][event]) gw->cbs[0][event](client, event_data);
            if (gw->cbs[1][event]) gw->cbs[1][event](client, event_data);
            discord_refcounter_decr(&client->refcounter, event_data);
        }
        break;
    case DISCORD_EV_NONE:
        logconf_warn(
            &gw->conf,
            "Expected unimplemented GATEWAY_DISPATCH event (code: %d)", event);
        break;
    }
}

void
discord_gateway_send_identify(struct discord_gateway *gw,
                              struct discord_identify *identify)
{
    struct ws_info info = { 0 };
    char buf[2056];
    jsonb b;

    /* Ratelimit check */
    if (gw->timer->now - gw->timer->identify_last < 5) {
        ++gw->session->concurrent;
        VASSERT_S(gw->session->concurrent
                      < gw->session->start_limit.max_concurrency,
                  "Reach identify request threshold (%d every 5 seconds)",
                  gw->session->start_limit.max_concurrency);
    }
    else {
        gw->session->concurrent = 0;
    }

    jsonb_init(&b);
    jsonb_object(&b, buf, sizeof(buf));
    {
        jsonb_key(&b, buf, sizeof(buf), "op", 2);
        jsonb_number(&b, buf, sizeof(buf), 2);
        jsonb_key(&b, buf, sizeof(buf), "d", 1);
        discord_identify_to_jsonb(&b, buf, sizeof(buf), identify);
        jsonb_object_pop(&b, buf, sizeof(buf));
    }

    if (ws_send_text(gw->ws, &info, buf, b.pos)) {
        io_poller_curlm_enable_perform(CLIENT(gw, gw)->io_poller, gw->mhandle);
        logconf_info(
            &gw->conf,
            ANSICOLOR(
                "SEND",
                ANSI_FG_BRIGHT_GREEN) " IDENTIFY (%d bytes) [@@@_%zu_@@@]",
            b.pos, info.loginfo.counter + 1);

        /* get timestamp for this identify */
        gw->timer->identify_last = gw->timer->now;
    }
    else {
        logconf_info(
            &gw->conf,
            ANSICOLOR("FAIL SEND",
                      ANSI_FG_RED) " IDENTIFY (%d bytes) [@@@_%zu_@@@]",
            b.pos, info.loginfo.counter + 1);
    }
}

void
discord_gateway_send_resume(struct discord_gateway *gw,
                            struct discord_resume *event)
{
    struct ws_info info = { 0 };
    char buf[1024];
    jsonb b;

    /* reset */
    gw->session->status ^= DISCORD_SESSION_RESUMABLE;

    jsonb_init(&b);
    jsonb_object(&b, buf, sizeof(buf));
    {
        jsonb_key(&b, buf, sizeof(buf), "op", 2);
        jsonb_number(&b, buf, sizeof(buf), 6);
        jsonb_key(&b, buf, sizeof(buf), "d", 1);
        discord_resume_to_jsonb(&b, buf, sizeof(buf), event);
        jsonb_object_pop(&b, buf, sizeof(buf));
    }

    if (ws_send_text(gw->ws, &info, buf, b.pos)) {
        io_poller_curlm_enable_perform(CLIENT(gw, gw)->io_poller, gw->mhandle);
        logconf_info(
            &gw->conf,
            ANSICOLOR("SEND",
                      ANSI_FG_BRIGHT_GREEN) " RESUME (%d bytes) [@@@_%zu_@@@]",
            b.pos, info.loginfo.counter + 1);
    }
    else {
        logconf_info(&gw->conf,
                     ANSICOLOR("FAIL SEND",
                               ANSI_FG_RED) " RESUME (%d bytes) [@@@_%zu_@@@]",
                     b.pos, info.loginfo.counter + 1);
    }
}

static void
_discord_on_heartbeat_timeout(struct discord *client,
                              struct discord_timer *timer)
{
    (void)client;
    struct discord_gateway *gw = timer->data;

    if (CCORD_OK == discord_gateway_perform(gw)
        && ~gw->session->status & DISCORD_SESSION_SHUTDOWN
        && gw->session->is_ready)
    {
        discord_gateway_send_heartbeat(gw, gw->payload.seq);
    }
    const u64unix_ms next_hb =
        gw->timer->hbeat_last + (u64unix_ms)gw->timer->hbeat_interval;

    timer->interval = (int64_t)(next_hb) - (int64_t)discord_timestamp(client);
    if (timer->interval < 1) timer->interval = 1;
    timer->repeat = 1;
}

/* send heartbeat pulse to websockets server in order
 *  to maintain connection alive */
void
discord_gateway_send_heartbeat(struct discord_gateway *gw, int seq)
{
    struct ws_info info = { 0 };
    char buf[64];
    jsonb b;

    jsonb_init(&b);
    jsonb_object(&b, buf, sizeof(buf));
    {
        jsonb_key(&b, buf, sizeof(buf), "op", 2);
        jsonb_number(&b, buf, sizeof(buf), 1);
        jsonb_key(&b, buf, sizeof(buf), "d", 1);
        jsonb_number(&b, buf, sizeof(buf), seq);
        jsonb_object_pop(&b, buf, sizeof(buf));
    }

    if (ws_send_text(gw->ws, &info, buf, b.pos)) {
        io_poller_curlm_enable_perform(CLIENT(gw, gw)->io_poller, gw->mhandle);
        logconf_info(
            &gw->conf,
            ANSICOLOR(
                "SEND",
                ANSI_FG_BRIGHT_GREEN) " HEARTBEAT (%d bytes) [@@@_%zu_@@@]",
            b.pos, info.loginfo.counter + 1);

        /* update heartbeat timestamp */
        gw->timer->hbeat_last = gw->timer->now;
        if (!gw->timer->hbeat_timer)
            gw->timer->hbeat_timer = discord_internal_timer(
                CLIENT(gw, gw), _discord_on_heartbeat_timeout, NULL, gw,
                gw->timer->hbeat_interval);
    }
    else {
        logconf_info(
            &gw->conf,
            ANSICOLOR("FAIL SEND",
                      ANSI_FG_RED) " HEARTBEAT (%d bytes) [@@@_%zu_@@@]",
            b.pos, info.loginfo.counter + 1);
    }
}

void
discord_gateway_send_request_guild_members(
    struct discord_gateway *gw, struct discord_request_guild_members *event)
{
    struct ws_info info = { 0 };
    char buf[4096];
    jsonb b;

    jsonb_init(&b);
    jsonb_object(&b, buf, sizeof(buf));
    {
        jsonb_key(&b, buf, sizeof(buf), "op", 2);
        jsonb_number(&b, buf, sizeof(buf), 8);
        jsonb_key(&b, buf, sizeof(buf), "d", 1);
        discord_request_guild_members_to_jsonb(&b, buf, sizeof(buf), event);
        jsonb_object_pop(&b, buf, sizeof(buf));
    }

    if (ws_send_text(gw->ws, &info, buf, b.pos)) {
        io_poller_curlm_enable_perform(CLIENT(gw, gw)->io_poller, gw->mhandle);
        logconf_info(
            &gw->conf,
            ANSICOLOR("SEND", ANSI_FG_BRIGHT_GREEN) " REQUEST_GUILD_MEMBERS "
                                                    "(%d bytes) [@@@_%zu_@@@]",
            b.pos, info.loginfo.counter + 1);
    }
    else {
        logconf_info(
            &gw->conf,
            ANSICOLOR(
                "FAIL SEND",
                ANSI_FG_RED) " REQUEST_GUILD_MEMBERS (%d bytes) [@@@_%zu_@@@]",
            b.pos, info.loginfo.counter + 1);
    }
}

void
discord_gateway_send_update_voice_state(
    struct discord_gateway *gw, struct discord_update_voice_state *event)
{
    struct ws_info info = { 0 };
    char buf[256];
    jsonb b;

    jsonb_init(&b);
    jsonb_object(&b, buf, sizeof(buf));
    {
        jsonb_key(&b, buf, sizeof(buf), "op", 2);
        jsonb_number(&b, buf, sizeof(buf), 4);
        jsonb_key(&b, buf, sizeof(buf), "d", 1);
        discord_update_voice_state_to_jsonb(&b, buf, sizeof(buf), event);
        jsonb_object_pop(&b, buf, sizeof(buf));
    }

    if (ws_send_text(gw->ws, &info, buf, b.pos)) {
        io_poller_curlm_enable_perform(CLIENT(gw, gw)->io_poller, gw->mhandle);
        logconf_info(
            &gw->conf,
            ANSICOLOR(
                "SEND",
                ANSI_FG_BRIGHT_GREEN) " UPDATE_VOICE_STATE "
                                      "(%d bytes): %s channels [@@@_%zu_@@@]",
            b.pos, event->channel_id ? "join" : "leave",
            info.loginfo.counter + 1);
    }
    else {
        logconf_info(
            &gw->conf,
            ANSICOLOR(
                "FAIL SEND",
                ANSI_FG_RED) " UPDATE_VOICE_STATE (%d bytes) [@@@_%zu_@@@]",
            b.pos, info.loginfo.counter + 1);
    }
}

void
discord_gateway_send_presence_update(struct discord_gateway *gw,
                                     struct discord_presence_update *presence)
{
    struct ws_info info = { 0 };
    char buf[2048];
    jsonb b;

    if (!gw->session->is_ready) return;

    jsonb_init(&b);
    jsonb_object(&b, buf, sizeof(buf));
    {
        jsonb_key(&b, buf, sizeof(buf), "op", 2);
        jsonb_number(&b, buf, sizeof(buf), 3);
        jsonb_key(&b, buf, sizeof(buf), "d", 1);
        discord_presence_update_to_jsonb(&b, buf, sizeof(buf), presence);
        jsonb_object_pop(&b, buf, sizeof(buf));
    }

    if (ws_send_text(gw->ws, &info, buf, b.pos)) {
        io_poller_curlm_enable_perform(CLIENT(gw, gw)->io_poller, gw->mhandle);
        logconf_info(
            &gw->conf,
            ANSICOLOR("SEND", ANSI_FG_BRIGHT_GREEN) " PRESENCE UPDATE (%d "
                                                    "bytes) [@@@_%zu_@@@]",
            b.pos, info.loginfo.counter + 1);
    }
    else {
        logconf_error(
            &gw->conf,
            ANSICOLOR("FAIL SEND", ANSI_FG_RED) " PRESENCE UPDATE (%d "
                                                "bytes) [@@@_%zu_@@@]",
            b.pos, info.loginfo.counter + 1);
    }
}
