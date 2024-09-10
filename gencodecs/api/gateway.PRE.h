/*****************************************************************************
 * Gateway Datatypes
 * **************************************************************************/

/** @defgroup DiscordInternalGatewayIntents Gateway intents
 * @brief Events user is interested on listening to
 * @ingroup DiscordInternalGateway
 *  @{ */
PP_DEFINE(DISCORD_GATEWAY_GUILDS 1 << 0)
PP_DEFINE(DISCORD_GATEWAY_GUILD_MEMBERS 1 << 1)
PP_DEFINE(DISCORD_GATEWAY_GUILD_BANS 1 << 2)
PP_DEFINE(DISCORD_GATEWAY_GUILD_EMOJIS_AND_STICKERS 1 << 3)
PP_DEFINE(DISCORD_GATEWAY_GUILD_INTEGRATIONS 1 << 4)
PP_DEFINE(DISCORD_GATEWAY_GUILD_WEBHOOKS 1 << 5)
PP_DEFINE(DISCORD_GATEWAY_GUILD_INVITES 1 << 6)
PP_DEFINE(DISCORD_GATEWAY_GUILD_VOICE_STATES 1 << 7)
PP_DEFINE(DISCORD_GATEWAY_GUILD_PRESENCES 1 << 8)
PP_DEFINE(DISCORD_GATEWAY_GUILD_MESSAGES 1 << 9)
PP_DEFINE(DISCORD_GATEWAY_GUILD_MESSAGE_REACTIONS 1 << 10)
PP_DEFINE(DISCORD_GATEWAY_GUILD_MESSAGE_TYPING 1 << 11)
PP_DEFINE(DISCORD_GATEWAY_DIRECT_MESSAGES 1 << 12)
PP_DEFINE(DISCORD_GATEWAY_DIRECT_MESSAGE_REACTIONS 1 << 13)
PP_DEFINE(DISCORD_GATEWAY_DIRECT_MESSAGE_TYPING 1 << 14)
PP_DEFINE(DISCORD_GATEWAY_MESSAGE_CONTENT 1 << 15)
PP_DEFINE(DISCORD_GATEWAY_GUILD_SCHEDULED_EVENTS 1 << 16)
PP_DEFINE(DISCORD_GATEWAY_AUTO_MODERATION_CONFIGURATION 1 << 20)
PP_DEFINE(DISCORD_GATEWAY_AUTO_MODERATION_EXECUTION 1 << 21)
/** @} DiscordInternalGatewayIntents */

/** @defgroup DiscordActivityFlags Gateway activity flags
 * @brief Describe user activities
 * @ingroup DiscordConstants
 *  @{ */
PP_DEFINE(DISCORD_ACTIVITY_INSTANCE 1 << 0)
PP_DEFINE(DISCORD_ACTIVITY_JOIN 1 << 1)
PP_DEFINE(DISCORD_ACTIVITY_SPECTATE 1 << 2)
PP_DEFINE(DISCORD_ACTIVITY_JOIN_REQUEST 1 << 3)
PP_DEFINE(DISCORD_ACTIVITY_SYNC 1 << 4)
PP_DEFINE(DISCORD_ACTIVITY_PLAY 1 << 5)
PP_DEFINE(DISCORD_ACTIVITY_PARTY_PRIVACY_FRIENDS 1 << 6)
PP_DEFINE(DISCORD_ACTIVITY_PARTY_PRIVACY_VOICE_CHANNEL 1 << 7)
PP_DEFINE(DISCORD_ACTIVITY_EMBEDDED 1 << 8)
/** @} DiscordActivityFlags */

#if GENCODECS_RECIPE == DATA
ENUM(discord_gateway_close_opcodes)
    ENUMERATOR(DISCORD_GATEWAY_CLOSE_REASON_UNKNOWN_ERROR, = 4000)
    ENUMERATOR(DISCORD_GATEWAY_CLOSE_REASON_UNKNOWN_OPCODE, = 4001)
    ENUMERATOR(DISCORD_GATEWAY_CLOSE_REASON_DECODE_ERROR, = 4002)
    ENUMERATOR(DISCORD_GATEWAY_CLOSE_REASON_NOT_AUTHENTICATED, = 4003)
    ENUMERATOR(DISCORD_GATEWAY_CLOSE_REASON_AUTHENTICATION_FAILED, = 4004)
    ENUMERATOR(DISCORD_GATEWAY_CLOSE_REASON_ALREADY_AUTHENTICATED, = 4005)
    ENUMERATOR(DISCORD_GATEWAY_CLOSE_REASON_INVALID_SEQUENCE, = 4007)
    ENUMERATOR(DISCORD_GATEWAY_CLOSE_REASON_RATE_LIMITED, = 4008)
    ENUMERATOR(DISCORD_GATEWAY_CLOSE_REASON_SESSION_TIMED_OUT, = 4009)
    ENUMERATOR(DISCORD_GATEWAY_CLOSE_REASON_INVALID_SHARD, = 4010)
    ENUMERATOR(DISCORD_GATEWAY_CLOSE_REASON_SHARDING_REQUIRED, = 4011)
    ENUMERATOR(DISCORD_GATEWAY_CLOSE_REASON_INVALID_API_VERSION, = 4012)
    ENUMERATOR(DISCORD_GATEWAY_CLOSE_REASON_INVALID_INTENTS, = 4013)
    ENUMERATOR(DISCORD_GATEWAY_CLOSE_REASON_DISALLOWED_INTENTS, = 4014)
    ENUMERATOR_LAST(DISCORD_GATEWAY_CLOSE_REASON_RECONNECT, = 4900)
ENUM_END
#endif

#if GENCODECS_RECIPE == DATA
ENUM(discord_gateway_opcodes)
    ENUMERATOR(DISCORD_GATEWAY_DISPATCH, = 0)
    ENUMERATOR(DISCORD_GATEWAY_HEARTBEAT, = 1)
    ENUMERATOR(DISCORD_GATEWAY_IDENTIFY, = 2)
    ENUMERATOR(DISCORD_GATEWAY_PRESENCE_UPDATE, = 3)
    ENUMERATOR(DISCORD_GATEWAY_VOICE_STATE_UPDATE, = 4)
    ENUMERATOR(DISCORD_GATEWAY_RESUME, = 6)
    ENUMERATOR(DISCORD_GATEWAY_RECONNECT, = 7)
    ENUMERATOR(DISCORD_GATEWAY_REQUEST_GUILD_MEMBERS, = 8)
    ENUMERATOR(DISCORD_GATEWAY_INVALID_SESSION, = 9)
    ENUMERATOR(DISCORD_GATEWAY_HELLO, = 10)
    ENUMERATOR_LAST(DISCORD_GATEWAY_HEARTBEAT_ACK, = 11)
ENUM_END
#endif

#if GENCODECS_RECIPE == DATA
ENUM(discord_activity_types)
  /** Format: \"Playing {name}\" */
    ENUMERATOR(DISCORD_ACTIVITY_GAME, = 0)
  /** Format: \"Streaming {details}\" */
    ENUMERATOR(DISCORD_ACTIVITY_STREAMING, = 1)
  /** Format: \"Listening to {name}\" */
    ENUMERATOR(DISCORD_ACTIVITY_LISTENING, = 2)
  /** Format: \"Watching {name}\" */
    ENUMERATOR(DISCORD_ACTIVITY_WATCHING, = 3)
  /** Format: \"{emoji} {name}\" */
    ENUMERATOR(DISCORD_ACTIVITY_CUSTOM, = 4)
  /** Format: \"Competing in {name}\" */
    ENUMERATOR_LAST(DISCORD_ACTIVITY_COMPETING, = 5)
ENUM_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_activity)
  /** the activity's name */
  COND_WRITE(self->name != NULL)
    FIELD_PTR(name, char, *)
  COND_END
  /** activity type */
    FIELD_ENUM(type, discord_activity_types)
  /** stream url, is validated when type is 1 */
  COND_WRITE(self->type == 1 && self->url != NULL)
    FIELD_PTR(url, char, *)
  COND_END
  /** unix timestamp (in milliseconds) of when the activity was added to the
       user's session */
  COND_WRITE(self->created_at != 0)
    FIELD_TIMESTAMP(created_at)
  COND_END
  /** unix timestamps for start and/or end of the game */
  COND_WRITE(self->timestamps != NULL)
    FIELD_STRUCT_PTR(timestamps, discord_activity_timestamps, *)
  COND_END
  /** application ID for the game */
  COND_WRITE(self->application_id != 0)
    FIELD_SNOWFLAKE(application_id)
  COND_END
  /** what the player is currently doing */
  COND_WRITE(self->details != NULL)
    FIELD_PTR(details, char, *)
  COND_END
  /** the user's current party status */
  COND_WRITE(self->state != NULL)
    FIELD_PTR(state, char, *)
  COND_END
  /** the emoji used for a custom status */
  COND_WRITE(self->emoji != NULL)
    FIELD_STRUCT_PTR(emoji, discord_activity_emoji, *)
  COND_END
  /** information for the current party of the player */
  COND_WRITE(self->party != NULL)
    FIELD_STRUCT_PTR(party, discord_activity_party, *)
  COND_END
  /** images for the presence and their hover texts */
  COND_WRITE(self->assets != NULL)
    FIELD_STRUCT_PTR(assets, discord_activity_assets, *)
  COND_END
  /** secrets for Rich Presence joining and spectating */
  COND_WRITE(self->secrets != NULL)
    FIELD_STRUCT_PTR(secrets, discord_activity_secrets, *)
  COND_END
  /** whether or not the activity is an instanced game session */
    FIELD(instance, bool, false)
  /** activity flags bitwise mask, describes what they payload includes
        @see @ref DiscordActivityFlags */
    FIELD_BITMASK(flags)
  /** the custom buttons shown in the Rich Presence (max 2) */
  COND_WRITE(self->buttons != NULL)
    FIELD_STRUCT_PTR(buttons, discord_activity_buttons, *)
  COND_END
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
LIST(discord_activities)
    LISTTYPE_STRUCT(discord_activity)
LIST_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_activity_timestamps)
  /** unix timestamp (in milliseconds)of when the activity started */
    FIELD_TIMESTAMP(start)
  /** unix timestamp (in milliseconds)of when the activity ends */
    FIELD_TIMESTAMP(end)
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_activity_emoji)
  /** the name of the emoji */
    FIELD_PTR(name, char, *)
  /** the ID of the emoji */
    FIELD_SNOWFLAKE(id)
  /** whether this emoji is animated */
    FIELD(animated, bool, false)
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_activity_party)
  /** the ID of the party */
    FIELD_PTR(id, char, *)
  /** used to show the party's current and maximum size @note array of two
       integers (current_size, max_size) */
    FIELD_STRUCT_PTR(size, integers, *)
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_activity_assets)
  /** activity large asset image */
    FIELD_PTR(large_image, char, *)
  /** text displayed when hovering over the large image of the activity */
    FIELD_PTR(large_text, char, *)
  /** activity small asset image */
    FIELD_PTR(small_image, char, *)
  /** text displayed when hovering over the small image of the activity */
    FIELD_PTR(small_text, char, *)
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_activity_secrets)
  /** the secret for joining a party */
    FIELD_PTR(join, char, *)
  /** the secret for spectating a game */
    FIELD_PTR(spectate, char, *)
  /** the secret for a specific instanced match */
    FIELD_PTR(match, char, *)
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_activity_button)
  /** the text shown on the button (1-32 characters) */
    FIELD_PTR(label, char, *)
  /** the url opened when clicking the button (1-512 characters) */
    FIELD_PTR(url, char, *)
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
LIST(discord_activity_buttons)
    LISTTYPE_STRUCT(discord_activity_button)
LIST_END
#endif

/** @CCORD_pub_struct{discord_presence_update} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_presence_update)
  /** the user presence is being updated for */
  COND_WRITE(self->user != NULL)
    FIELD_STRUCT_PTR(user, discord_user, *)
  COND_END
  /** id of the guild */
  COND_WRITE(self->guild_id != 0)
    FIELD_SNOWFLAKE(guild_id)
  COND_END
  /** either "idle", "dnd", "online", or "offline" */
  COND_WRITE(self->status != NULL)
    FIELD_PTR(status, char, *)
  COND_END
  /** user's platform-dependent status */
  COND_WRITE(self->client_status != NULL)
    FIELD_STRUCT_PTR(client_status, discord_client_status, *)
  COND_END
  /** user's current activities */
  COND_WRITE(self->activities != NULL)
    FIELD_STRUCT_PTR(activities, discord_activities, *)
  COND_END
  /** unix time (in milliseconds) of when the client went idle, or null if
       the client is not idle */
  COND_WRITE(self->since != 0)
    FIELD_TIMESTAMP(since)
  COND_END
  /** whether or not the client is afk */
    FIELD(afk, bool, false)
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_client_status)
  /** the user's status set for an active desktop (Windows, Linux, Mac) 
   *    application session */
    FIELD_PTR(desktop, char, *)
  /** the user's status set for an active mobile (iOS, Android) application
   *    session */
    FIELD_PTR(mobile, char, *)
  /** the user's status set for an active web (browser, bot account)
   *    application session */
    FIELD_PTR(web, char, *)
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
LIST(discord_presence_updates)
    LISTTYPE_STRUCT(discord_presence_update)
LIST_END
#endif

/* GATEWAY COMMAND PAYLOADS ONLY NEED TO BE ENCODED INTO JSON */

/** @CCORD_pub_struct{discord_identify} */
#if GENCODECS_RECIPE & (DATA | JSON_ENCODER)
PUB_STRUCT(discord_identify)
  /** authentication token */
    FIELD_PTR(token, char, *)
  /** connection properties */
    FIELD_STRUCT_PTR(properties, discord_identify_connection, *)
  /** whether this connection supports compression packets */
    FIELD(compress, bool, false)
  /** value between 50 and 250, total number of members where the gateway
       will stop sending offline members in the guild member list */
    FIELD(large_threshold, int, 50)
  /** array of two integers (shard_id, num_shards) */
  COND_WRITE(self->shard != NULL)
    FIELD_STRUCT_PTR(shard, integers, *)
  COND_END
  /** presence structure for initial presence information */
  COND_WRITE(self->presence != NULL)
    FIELD_STRUCT_PTR(presence, discord_presence_update, *)
  COND_END
  /** the gateway intents you wish to receive
       @see @ref DiscordInternalGatewayIntents */
    FIELD_BITMASK(intents)
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON_ENCODER)
STRUCT(discord_identify_connection)
  /** your operating system */
    FIELD_PTR(os, char, *)
  /** your library name */
    FIELD_PTR(browser, char, *)
  /** your library name */
    FIELD_PTR(device, char, *)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_resume} */
#if GENCODECS_RECIPE & (DATA | JSON_ENCODER)
PUB_STRUCT(discord_resume)
  /** session token */
    FIELD_PTR(token, char, *)
  /** session id */
    FIELD_PTR(session_id, char, *)
  /** last sequence number received */
    FIELD(seq, int, 0)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_request_guild_members} */
#if GENCODECS_RECIPE & (DATA | JSON_ENCODER)
PUB_STRUCT(discord_request_guild_members)
  /** id of the guild to get members for */
    FIELD_SNOWFLAKE(guild_id)
  /** string that username starts with, or an empty string to return all
   *    members */
    FIELD_PTR(query, char, *)
  /** maximum number of members to send matching the `query`; a limit of `0`
   *    can be used with an empty string `query` to return all members */
    FIELD(limit, int, 0)
  /** used to specify if we want the presences of the matched members */
    FIELD(presences, bool, false)
  /** used to specify which users you wish to fetch */
  COND_WRITE(self->user_ids != NULL)
    FIELD_STRUCT_PTR(user_ids, snowflakes, *)
  COND_END
  /** nonce to identify the `Guild Members Chunk` response */
  COND_WRITE(self->nonce != NULL)
    FIELD_PTR(nonce, char, *)
  COND_END
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_update_voice_state} */
#if GENCODECS_RECIPE & (DATA | JSON_ENCODER)
PUB_STRUCT(discord_update_voice_state)
  /** ID of the guild */
    FIELD_SNOWFLAKE(guild_id)
  /** ID of the voice channel client wants to join (null if disconnecting) */
    FIELD_SNOWFLAKE(channel_id)
  /** is the client muted */
    FIELD(self_mute, bool, false)
  /** is the client deafened */
    FIELD(self_deaf, bool, false)
STRUCT_END
#endif

/* EVENT PAYLOADS ONLY NEED TO BE DECODED INTO STRUCTS */

/** @CCORD_pub_struct{discord_ready} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_ready)
  /** gateway version */
    FIELD(v, int, 0)
  /** information about the user including email */
    FIELD_STRUCT_PTR(user, discord_user, *)
  /** the guilds the user is in */
    FIELD_STRUCT_PTR(guilds, discord_guilds, *)
  /** used for resuming connections */
    FIELD_PTR(session_id, char, *)
  /** the shard information associated with this session, if sent when
   *        identifying*/
    FIELD_STRUCT_PTR(shard, integers, *)
  /** contains `id` and `flags` */
    FIELD_STRUCT_PTR(application, discord_application, *)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_auto_moderation_action_execution} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_auto_moderation_action_execution)
  /** the id of the guild in which action was executed */
    FIELD_SNOWFLAKE(guild_id)
  /** the action which was executed */
    FIELD_STRUCT_PTR(action, discord_auto_moderation_action, *)
  /** the trigger typeof rule which was triggered */
    FIELD_ENUM(rule_trigger_type, discord_auto_moderation_trigger_types)
  /** the id of the user which generated the content which triggered the rule */
    FIELD_SNOWFLAKE(user_id)
  /** the id of the channel in which user content was posted */
    FIELD_SNOWFLAKE(channel_id)
  /** the id of any user message which content belongs to */
    FIELD_SNOWFLAKE(message_id)
  /**
   * the id of any system auto moderation messages posted as a result of 
   *        this action
   */
    FIELD_SNOWFLAKE(alert_system_message_id)
  /** the user generated text content */
    FIELD_PTR(content, char, *)
  /** the word or phrase configured in the rule that triggered the rule */
    FIELD_PTR(matched_keyword, char, *)
  /** the substring in content that triggered the rule */
    FIELD_PTR(matched_content, char, *)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_thread_list_sync} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_thread_list_sync)
  /** the id of the guild */
    FIELD_SNOWFLAKE(guild_id)
  /** 
   * the parent channel ids whose threads are being synced. if omitted, then
   *        threads were synced for the entire guild. This array may contain 
   *        channel_ids that have no active threads as well, so you know to
   *        clear data
   */
    FIELD_STRUCT_PTR(channel_ids, snowflakes, *)
  /** all active threads in the given channels that the current user can access */
    FIELD_STRUCT_PTR(threads, discord_channels, *)
  /** all thread member objects from the synced threads for the current user,
   *    indicating which threads the current user has been added to */
    FIELD_STRUCT_PTR(members, discord_thread_members, *)
STRUCT_END
#endif

/** 
 * @CCORD_pub_struct{discord_thread_members_update}
 * @todo `added_members` may include guild_members and presence objects
 */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_thread_members_update)
  /** the id of the thread */
    FIELD_SNOWFLAKE(id)
  /** the id of the guild */
    FIELD_SNOWFLAKE(guild_id)
  /** the approximate number of members in the thread, capped at 50 */
    FIELD(member_count, int, 0)
  /** the users who were added to the thread */
    FIELD_STRUCT_PTR(added_members, discord_thread_members, *)
  /** the id of the users who were removed from the thread */
    FIELD_STRUCT_PTR(removed_member_ids, snowflakes, *)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_channel_pins_update} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_channel_pins_update)
  /** the id of the guild */
    FIELD_SNOWFLAKE(guild_id)
  /** the id of the channel */
    FIELD_SNOWFLAKE(channel_id)
  /** the time at which the most recent pinned message was pinned */
    FIELD_TIMESTAMP(last_pin_timestamp)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_thread_create} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_thread_create)
  /** the ID of this channel */
    FIELD_SNOWFLAKE(id)
  /** the type of channel */
    FIELD_ENUM(type, discord_channel_types)
  /** the ID of the guild (may be missing for some channel objects received
       over gateway guild dispatches) */
  COND_WRITE(self->guild_id != 0)
    FIELD_SNOWFLAKE(guild_id)
  COND_END
  /** sorting position of the channel */
    FIELD(position, int, 0)
  /** explicit permission overwrites for members and roles */
    FIELD_STRUCT_PTR(permission_overwrites, discord_overwrites, *)
  /** the name of the channel (0-1024 characters) */
    FIELD_PTR(name, char, *)
  /** the channel topic (0-1024 characters) */
    FIELD_PTR(topic, char, *)
  /** whether the channel is nsfw */
    FIELD(nsfw, bool, false)
  /** the id of the last message sent in this channel (may not point to an
       existing or valid message) */
    FIELD_SNOWFLAKE(last_message_id)
  /** the bitrate (in bits) of the voice channel */
    FIELD(bitrate, int, 0)
  /** the user limit of the voice channel */
    FIELD(user_limit, int, 0)
  /** amount of seconds a user has to wait before sending another message
        (0-21600). bots, as well as users with the permission
       `MANAGE_MESSAGES` or `MANAGE_CHANNEL` are unaffected */
    FIELD(rate_limit_per_user, int, 0)
  /** array of user objects */
    FIELD_STRUCT_PTR(recipients, discord_users, *)
  /** icon hash of the group DM */
    FIELD_PTR(icon, char, *)
  /** id of the creator of the group DM or thread */
    FIELD_SNOWFLAKE(owner_id)
  /** application ID of the group DM creator if its bot-created */
    FIELD_SNOWFLAKE(application_id)
  /** for group DM channels: whether the channel is managed by an application
        via the gdm.join OAuth2 scope */
    FIELD(managed, bool, false)
  /** for guild channels: ID of the parent category for a channel (each
       parent category can contain up to 50 channels), for threads: id of
       the text channel this thread was created */
    FIELD_SNOWFLAKE(parent_id)
  /** when the last pinned message was pinned. This may be `NULL` in events
       such as `GUILD_CREATE` when a message is not pinned */
    FIELD_TIMESTAMP(last_pin_timestamp)
  /** voice region ID for the voice channel, automatic when set to null */
    FIELD_PTR(rtc_region, char, *)
  /** the camera video quality mode of the voice channel, 1 when not present */
    FIELD(voice_quality_mode, int, 0)
  /** an approximate count of messages in a thread, stops counting at 50 */
    FIELD(message_count, int, 0)
  /** an approximate count of users in a thread, stops counting at 50 */
    FIELD(member_count, int, 0)
  /** thread-specific fields not needed by other channels */
    FIELD_STRUCT_PTR(thread_metadata, discord_thread_metadata, *)
  /** thread member object for the current user, if they have joined the
       thread, only included on certain API endpoints */
    FIELD_STRUCT_PTR(member, discord_thread_member, *)
  /** default duration that the clients (not the API) will use for newly
       created threads, in minutes, to automatically archive the thread
       after recent activity, can be set to: 60, 1440, 4320, 10080 */
    FIELD(default_auto_archive_duration, int, 0)
  /** computed permissions for the invoking user in the channel, including
       overwrites, only included when part of the `resolved` data received
       on a application command interaction */
    FIELD_BITMASK(permissions)
  /** 	channel flags combined as a bitfield */
    FIELD_BITMASK(flags)
  /** number of messages ever sent in a thread */
    FIELD(total_message_sent, int, 0)
  /** the set of tags that can be used in a GUILD_FORUM or a GUILD_MEDIA channel */
    FIELD_STRUCT_PTR(available_tags, discord_thread_tags, *)
  /** the IDs of the set of tags that have been applied to a thread
        in a GUILD_FORUM or a GUILD_MEDIA channel */
    FIELD_STRUCT_PTR(applied_tags, snowflakes, *)
  /** the emoji to show in the add reaction button on a thread
        in a GUILD_FORUM or a GUILD_MEDIA channel */
    FIELD_STRUCT_PTR(default_reaction_emoji, discord_thread_default_reaction, *)
  /** the initial rate_limit_per_user to set on newly created threads in a channel */
    FIELD(default_thread_rate_limit_per_user, int, 0)
  /** the default sort order type used to order posts in GUILD_FORUM
        and GUILD_MEDIA channels */
    FIELD_ENUM(default_sort_order, discord_sort_order_types)
  /** he default forum layout view used to display posts in GUILD_FORUM channels */
    FIELD_ENUM(default_forum_layout, discord_forum_layout_types)
  /** Whether tdiscord_channel)he thread has been just created */
    FIELD(newly_created, bool, false)
  /**/
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_guild_ban_add} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_guild_ban_add)
  /** the id of the guild */
    FIELD_SNOWFLAKE(guild_id)
  /** the banned user */
    FIELD_STRUCT_PTR(user, discord_user, *)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_guild_ban_remove} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_guild_ban_remove)
  /** the id of the guild */
    FIELD_SNOWFLAKE(guild_id)
  /** the unbanned user */
    FIELD_STRUCT_PTR(user, discord_user, *)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_guild_emojis_update} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_guild_emojis_update)
  /** the id of the guild */
    FIELD_SNOWFLAKE(guild_id)
  /** array of emojis */
    FIELD_STRUCT_PTR(emojis, discord_emojis, *)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_stickers_update} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_guild_stickers_update)
  /** id of the guild */
    FIELD_SNOWFLAKE(guild_id)
  /** array of stickers */
    FIELD_STRUCT_PTR(stickers, discord_stickers, *)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_guild_integrations_update} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_guild_integrations_update)
  /** id of the guild whose integrations were updated */
    FIELD_SNOWFLAKE(guild_id)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_guild_member_remove} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_guild_member_remove)
  /** id of the guild */
    FIELD_SNOWFLAKE(guild_id)
  /** the user who was removed */
    FIELD_STRUCT_PTR(user, discord_user, *)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_guild_member_update} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_guild_member_update)
  /** id of the guild */
    FIELD_SNOWFLAKE(guild_id)
  /** user role ids */
    FIELD_STRUCT_PTR(roles, snowflakes, *)
  /** the user */
    FIELD_STRUCT_PTR(user, discord_user, *)
  /** nickname of the user in the guild */
    FIELD_PTR(nick, char, *)
  /** the member's guild avatar hash */
    FIELD_PTR(avatar, char, *)
  /** when the user joined the guild */
    FIELD_TIMESTAMP(joined_at)
  /** when the user started boosting the guild */
    FIELD_TIMESTAMP(premium_since)
  /** whether the user is deafened in voice channels */
    FIELD(deaf, bool, false)
  /** whether the user is muted in voice channels */
    FIELD(mute, bool, false)
  /** whether the user has not yet passed the guild's `Membership Screening`
   *    requirements */
    FIELD(pending, bool, false)
  /** 
   * when the user's timeout will expire and the user will be able to
   *    communicate in the guild again, `NULL` or a time in the past if the
   *    user is not timed out */
    FIELD_TIMESTAMP(communication_disabled_until)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_guild_members_chunk} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_guild_members_chunk)
  /** the id of the guild */
    FIELD_SNOWFLAKE(guild_id)
  /** set of guild members */
    FIELD_STRUCT_PTR(members, discord_guild_members, *)
  /** the chunk index in the expected chunks for this response
   *  @note `0 <= chunk_index < chunk_count` */
    FIELD(chunk_index, int, 0)
  /** the total number of expected chunks for this response */
    FIELD(chunk_count, int, 0)
  /** if passing an invalid id to `REQUEST_GUILD_MEMBERS`, it will be returned
   *    here */
    FIELD_STRUCT_PTR(not_found, snowflakes, *)
  /** if passing true to `REQUEST_GUILD_MEMBERS`, presences of the returned
   *    members will be here */
    FIELD_STRUCT_PTR(presences, discord_presence_updates, *)
  /** the nonce used in the `Guild Members Request` */
    FIELD_PTR(nonce, char, *)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_guild_role_create} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_guild_role_create)
  /** the id of the guild */
    FIELD_SNOWFLAKE(guild_id)
  /** the role created */
    FIELD_STRUCT_PTR(role, discord_role, *)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_guild_role_update} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_guild_role_update)
  /** the id of the guild */
    FIELD_SNOWFLAKE(guild_id)
  /** the role updated */
    FIELD_STRUCT_PTR(role, discord_role, *)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_guild_role_delete} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_guild_role_delete)
  /** the id of the guild */
    FIELD_SNOWFLAKE(guild_id)
  /** the id of the role */
    FIELD_SNOWFLAKE(role_id)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_scheduled_event_user_add} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_guild_scheduled_event_user_add)
  /** id of the guild scheduled event */
    FIELD_SNOWFLAKE(guild_scheduled_event_id)
  /** id of the user */
    FIELD_SNOWFLAKE(user_id)
  /** id of the guild */
    FIELD_SNOWFLAKE(guild_id)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_scheduled_event_user_remove} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_guild_scheduled_event_user_remove)
  /** id of the guild scheduled event */
    FIELD_SNOWFLAKE(guild_scheduled_event_id)
  /** id of the user */
    FIELD_SNOWFLAKE(user_id)
  /** id of the guild */
    FIELD_SNOWFLAKE(guild_id)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_integration_delete} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_integration_delete)
  /** integration id */
    FIELD_SNOWFLAKE(id)
  /** id of the guild */
    FIELD_SNOWFLAKE(guild_id)
  /** id of the bot/OAuth2 application for this Discord integration */
    FIELD_SNOWFLAKE(application_id)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_invite_create} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_invite_create)
  /** the channel the invite is for */
    FIELD_SNOWFLAKE(channel_id)
  /** the unique invite code */
    FIELD_PTR(code, char, *)
  /** the time at which the invite was created */
    FIELD_TIMESTAMP(created_at)
  /** the guild of the invite */
    FIELD_SNOWFLAKE(guild_id)
  /** the user that created the invite */
    FIELD_STRUCT_PTR(inviter, discord_user, *)
  /** how long the inviteis valid for (in seconds) */
    FIELD(max_age, int, 0)
  /** the maximum number of times the invite can be used */
    FIELD(max_uses, int, 0)
  /** the @ref discord_invite_target_types for this voice channel invite */
    FIELD_ENUM(target_type, discord_invite_target_types)
  /** the user whose stream to display for this voice channel stream invite */
    FIELD_STRUCT_PTR(target_user, discord_user, *)
  /** the embedded application to open for this voice channel embedded 
   *    application invite*/
    FIELD_STRUCT_PTR(target_application, discord_application, *)
  /** whether or not the invite is temporary (invited users will be kicked
   *    on disconnect unless they're assigned a role) */
    FIELD(temporary, bool, false)
  /** how many times the invite has been used (always 0) */
    FIELD(uses, int, 0)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_invite_delete} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_invite_delete)
  /** the channel of the invite */
    FIELD_SNOWFLAKE(channel_id)
  /** the guild of the invite */
    FIELD_SNOWFLAKE(guild_id)
  /** the unique invite code */
    FIELD_PTR(code, char, *)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_message_delete} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_message_delete)
  /** the id of the message */
    FIELD_SNOWFLAKE(id)
  /** the id of the channel */
    FIELD_SNOWFLAKE(channel_id)
  /** the id of the guild */
    FIELD_SNOWFLAKE(guild_id)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_message_delete_bulk} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_message_delete_bulk)
  /** the ids of the messages */
    FIELD_STRUCT_PTR(ids, snowflakes, *)
  /** the id of the channel */
    FIELD_SNOWFLAKE(channel_id)
  /** the id of the guild */
    FIELD_SNOWFLAKE(guild_id)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_message_reaction_add} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_message_reaction_add)
  /** the id of the user */
    FIELD_SNOWFLAKE(user_id)
  /** the id of the channel */
    FIELD_SNOWFLAKE(channel_id)
  /** the id of the message */
    FIELD_SNOWFLAKE(message_id)
  /** the id of the guild */
    FIELD_SNOWFLAKE(guild_id)
  /** the member who reacted if this happened in a guild */
    FIELD_STRUCT_PTR(member, discord_guild_member, *)
  /** the emoji used to react */
    FIELD_STRUCT_PTR(emoji, discord_emoji, *)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_message_reaction_remove} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_message_reaction_remove)
  /** the id of the user */
    FIELD_SNOWFLAKE(user_id)
  /** the id of the channel */
    FIELD_SNOWFLAKE(channel_id)
  /** the id of the message */
    FIELD_SNOWFLAKE(message_id)
  /** the id of the guild */
    FIELD_SNOWFLAKE(guild_id)
  /** the emoji used to react */
    FIELD_STRUCT_PTR(emoji, discord_emoji, *)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_message_reaction_remove_all} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_message_reaction_remove_all)
  /** the id of the channel */
    FIELD_SNOWFLAKE(channel_id)
  /** the id of the message */
    FIELD_SNOWFLAKE(message_id)
  /** the id of the guild */
    FIELD_SNOWFLAKE(guild_id)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_message_reaction_remove_emoji} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_message_reaction_remove_emoji)
  /** the id of the channel */
    FIELD_SNOWFLAKE(channel_id)
  /** the id of the guild */
    FIELD_SNOWFLAKE(guild_id)
  /** the id of the message */
    FIELD_SNOWFLAKE(message_id)
  /** the emoji that was removed */
    FIELD_STRUCT_PTR(emoji, discord_emoji, *)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_typing_start} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_typing_start)
  /** id of the channel */
    FIELD_SNOWFLAKE(channel_id)
  /** id of the guild */
    FIELD_SNOWFLAKE(guild_id)
  /** id of the user */
    FIELD_SNOWFLAKE(user_id)
  /** unix time (in seconds) of when the user started typing */
    FIELD_TIMESTAMP(timestamp)
  /** the member who started typing if this happened in a guild */
    FIELD_STRUCT_PTR(member, discord_guild_member, *)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_voice_server_update} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_voice_server_update)
  /** voice connection token */
    FIELD_PTR(token, char, *)
  /** the guild this voice server update is for */
    FIELD_SNOWFLAKE(guild_id)
  /** the voice server host */
    FIELD_PTR(endpoint, char, *)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_webhooks_update} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_webhooks_update)
  /** id of the guild */
    FIELD_SNOWFLAKE(guild_id)
  /** id of the channel */
    FIELD_SNOWFLAKE(channel_id)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_session_start_limit} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_session_start_limit)
  /** the total number of session starts the current user is allowed */
    FIELD(total, int, 0)
  /** the remaining number of session starts the current user is allowed */
    FIELD(remaining, int, 0)
  /** the number of milliseconds after which the limit resets */
    FIELD(reset_after, int, 0)
  /** the number of identify requests allowed per 5 seconds */
    FIELD(max_concurrency, int, 0)
STRUCT_END
#endif
