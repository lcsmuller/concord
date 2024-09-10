/**
 * @file discord-response.h
 * @author Cogmasters
 * @brief Generic macros for initializing a @ref discord_response and return
 *      handles
 */

#ifndef DISCORD_RESPONSE_H
#define DISCORD_RESPONSE_H

/** @brief The response for the completed request */
struct discord_response {
    /** user arbitrary data provided at @ref discord_ret */
    void *data;
    /** kept concord's parameter provided at @ref discord_ret */
    const void *keep;
    /** request completion status @see @ref ConcordError */
    CCORDcode code;
};

/******************************************************************************
 * Templates for generating type-safe return handles for async requests
 ******************************************************************************/

/**
 * @brief Macro containing common fields for `struct discord_ret*` datatypes
 * @note this exists for alignment purposes
 */
#define DISCORD_RET_DEFAULT_FIELDS                                            \
    /** user arbitrary data to be passed to `done` or `fail` callbacks */     \
    void *data;                                                               \
    /** cleanup method to be called for `data`, once its no longer            \
    being referenced */                                                       \
    void (*cleanup)(struct discord * client, void *data);                     \
    /** Concord callback parameter the client wish to keep reference */       \
    const void *keep;                                                         \
    /** if `true` then request will be prioritized over already enqueued      \
        requests */                                                           \
    bool high_priority;                                                       \
    /** optional callback to be executed on a failed request */               \
    void (*fail)(struct discord * client, struct discord_response * resp)

#define DISCORD_RETURN(_type)                                                 \
    /** @brief Request's return context */                                    \
    struct discord_ret_##_type {                                              \
        DISCORD_RET_DEFAULT_FIELDS;                                           \
        /** optional callback to be executed on a successful request */       \
        void (*done)(struct discord * client,                                 \
                     struct discord_response *resp,                           \
                     const struct discord_##_type *ret);                      \
        /** if an address is provided, then request will block the thread and \
           perform on-spot.                                                   \
           On success the response object will be written to the address,     \
           unless enabled with @ref DISCORD_SYNC_FLAG */                      \
        struct discord_##_type *sync;                                         \
    }

/** @brief Request's return context */
struct discord_ret {
    DISCORD_RET_DEFAULT_FIELDS;
    /** optional callback to be executed on a successful request */
    void (*done)(struct discord *client, struct discord_response *resp);
    /** if `true`, request will block the thread and perform on-spot */
    bool sync;
};

/** @brief flag for enabling `sync` mode without expecting a datatype return */
#define DISCORD_SYNC_FLAG ((void *)-1)

/** @addtogroup DiscordAPIOAuth2
 *  @{ */
DISCORD_RETURN(application);
DISCORD_RETURN(auth_response);
/** @} DiscordAPIOAuth2 */

/** @addtogroup DiscordAPIAuditLog
 *  @{ */
DISCORD_RETURN(audit_log);
/** @} DiscordAPIAuditLog */

/** @addtogroup DiscordAPIAutoModeration
 *  @{ */
DISCORD_RETURN(auto_moderation_rule);
DISCORD_RETURN(auto_moderation_rules);
/** @} DiscordAPIAutoModeration */

/** @addtogroup DiscordAPIChannel
 *  @{ */
DISCORD_RETURN(channel);
DISCORD_RETURN(channels);
DISCORD_RETURN(message);
DISCORD_RETURN(messages);
DISCORD_RETURN(followed_channel);
DISCORD_RETURN(thread_members);
DISCORD_RETURN(thread_response_body);
/** @} DiscordAPIChannel */

/** @addtogroup DiscordAPIEmoji
 *  @{ */
DISCORD_RETURN(emoji);
DISCORD_RETURN(emojis);
/** @} DiscordAPIEmoji */

/** @addtogroup DiscordAPIGuild
 *  @{ */
DISCORD_RETURN(guild);
DISCORD_RETURN(guilds);
DISCORD_RETURN(guild_preview);
DISCORD_RETURN(guild_member);
DISCORD_RETURN(guild_members);
DISCORD_RETURN(guild_widget);
DISCORD_RETURN(guild_widget_settings);
DISCORD_RETURN(ban);
DISCORD_RETURN(bans);
DISCORD_RETURN(role);
DISCORD_RETURN(roles);
DISCORD_RETURN(welcome_screen);
DISCORD_RETURN(integrations);
DISCORD_RETURN(prune_count);
/** @} DiscordAPIGuild */

/** @addtogroup DiscordAPIGuildScheduledEvent
 *  @{ */
DISCORD_RETURN(guild_scheduled_event);
DISCORD_RETURN(guild_scheduled_events);
DISCORD_RETURN(guild_scheduled_event_users);
/** @} DiscordAPIGuildScheduledEvent */

/** @addtogroup DiscordAPIGuildTemplate
 *  @{ */
DISCORD_RETURN(guild_template);
DISCORD_RETURN(guild_templates);
/** @} DiscordAPIGuildTemplate */

/** @addtogroup DiscordAPIInvite
 *  @{ */
DISCORD_RETURN(invite);
DISCORD_RETURN(invites);
/** @} DiscordAPIInvite */

/** @addtogroup DiscordAPIStageInstance
 *  @{ */
DISCORD_RETURN(stage_instance);
/** @} DiscordAPIStageInstance */

/** @addtogroup DiscordAPISticker
 *  @{ */
DISCORD_RETURN(sticker);
DISCORD_RETURN(stickers);
DISCORD_RETURN(list_nitro_sticker_packs);
/** @} DiscordAPISticker */

/** @addtogroup DiscordAPIUser
 *  @{ */
DISCORD_RETURN(user);
DISCORD_RETURN(users);
DISCORD_RETURN(connections);
/** @} DiscordAPIUser */

/** @addtogroup DiscordAPIVoice
 *  @{ */
DISCORD_RETURN(voice_regions);
/** @} DiscordAPIVoice */

/** @addtogroup DiscordAPIWebhook
 *  @{ */
DISCORD_RETURN(webhook);
DISCORD_RETURN(webhooks);
/** @} DiscordAPIWebhook */

/** @addtogroup DiscordAPIInteractionsApplicationCommand
 * @ingroup DiscordAPIInteractions
 *  @{ */
DISCORD_RETURN(application_command);
DISCORD_RETURN(application_commands);
DISCORD_RETURN(application_command_permission);
DISCORD_RETURN(application_command_permissions);
DISCORD_RETURN(guild_application_command_permissions);
/** @} DiscordAPIInteractionsApplicationCommand */

/** @addtogroup DiscordAPIInteractionsReact
 * @ingroup DiscordAPIInteractions
 *  @{ */
DISCORD_RETURN(interaction_response);
/** @} DiscordAPIInteractionsReact */

/** @addtogroup DiscordAPIPolls
 * @{ */
DISCORD_RETURN(poll_answer_voters);
/** @} DiscordAPIPolls */

#endif /* DISCORD_RESPONSE_H */
