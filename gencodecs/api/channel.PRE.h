/*****************************************************************************
 * Channel Datatypes
 * **************************************************************************/

#if GENCODECS_RECIPE == DATA
ENUM(discord_channel_types)
  /** a text channel within a server */
    ENUMERATOR(DISCORD_CHANNEL_GUILD_TEXT, = 0)
  /** a direct message between users */
    ENUMERATOR(DISCORD_CHANNEL_DM, = 1)
  /** a voice channel within a server */
    ENUMERATOR(DISCORD_CHANNEL_GUILD_VOICE, = 2)
  /** a direct message between multiple users */
    ENUMERATOR(DISCORD_CHANNEL_GROUP_DM, = 3)
  /** an organization category that contains up to 50 channels */
    ENUMERATOR(DISCORD_CHANNEL_GUILD_CATEGORY, = 4)
  /** a channel that users can follow and crosspost into their own server */
    ENUMERATOR(DISCORD_CHANNEL_GUILD_ANNOUNCEMENT, = 5)
  /** alias to @ref DISCORD_CHANNEL_GUILD_ANNOUNCEMENT */
    ENUMERATOR(DISCORD_CHANNEL_GUILD_NEWS, = DISCORD_CHANNEL_GUILD_ANNOUNCEMENT)
  /** a channel in which game developers can seel their game on Discord */
    ENUMERATOR(DISCORD_CHANNEL_GUILD_STORE, = 6)
  /** a temporary sub-channel within a @ref DISCORD_CHANNEL_GUILD_NEWS
       channel */
    ENUMERATOR(DISCORD_CHANNEL_ANNOUNCEMENT_THREAD, = 10)
  /** alias to @ref DISCORD_CHANNEL_ANNOUNCEMENT_THREAD */
    ENUMERATOR(DISCORD_CHANNEL_GUILD_NEWS_THREAD, = DISCORD_CHANNEL_ANNOUNCEMENT_THREAD)
  /** a temporary sub-channel within a @ref DISCORD_CHANNEL_GUILD_TEXT
       channel */
    ENUMERATOR(DISCORD_CHANNEL_GUILD_PUBLIC_THREAD, = 11)
  /** a temporary sub-channel within a @ref DISCORD_CHANNEL_GUILD_TEXT
       channel that is only viewable by those invited and those with the
       `MANAGE_THREADS` permission */
    ENUMERATOR(DISCORD_CHANNEL_GUILD_PRIVATE_THREAD, = 12)
  /** a voice channel for hosting events with an audience */
    ENUMERATOR(DISCORD_CHANNEL_GUILD_STAGE_VOICE, = 13)
  /** the channel in a hub containing the listed servers */
    ENUMERATOR(DISCORD_CHANNEL_GUILD_DIRECTORY, = 14)
  /** a channel that can only contain threads */
    ENUMERATOR(DISCORD_CHANNEL_GUILD_FORUM, = 15)
  /** Channel that can only contain threads, similar to DISCORD_CHANNEL_GUILD_FORUM channels */
    ENUMERATOR_LAST(DISCORD_CHANNEL_GUILD_MEDIA, = 16)
ENUM_END
#endif

#if GENCODECS_RECIPE == DATA
ENUM(discord_video_quality_modes)
  /** Discord chooses the quality for optimal performance */
    ENUMERATOR(DISCORD_VIDEO_QUALITY_AUTO, = 1)
  /** 720p */
    ENUMERATOR_LAST(DISCORD_VIDEO_QUALITY_FULL, = 2)
ENUM_END
#endif

/** @defgroup DiscordAPIChannelFlags Message flags
 * @ingroup DiscordAPIChannel
 *  @{ */

/** this thread is pinned to the top of its parent GUILD_FORUM 
   or GUILD_MEDIA channel */
PP_DEFINE(DISCORD_CHANNEL_PINNED 1 << 1)
/** whether a tag is required to be specified when creating a thread
   in a GUILD_FORUM or a GUILD_MEDIA channel */
PP_DEFINE(DISCORD_CHANNEL_REQUIRE_TAG 1 << 4)
/** when set hides the embedded media download options */
PP_DEFINE(DISCORD_CHANNEL_HIDE_MEDIA_DOWNLOAD_OPTIONS 1 << 5)

/** @} DiscordAPIChannelFlags */

#if GENCODECS_RECIPE == DATA
ENUM(discord_sort_order_types)
  /** sort forum posts by activity */
    ENUMERATOR(DISCORD_SORT_ORDER_LATEST_ACTIVITY, = 0)
  /** sort forum posts by creation time (from most recent to oldest) */
    ENUMERATOR_LAST(DISCORD_SORT_ORDER_CREATION_DATE, = 1)
ENUM_END
#endif

#if GENCODECS_RECIPE == DATA
ENUM(discord_forum_layout_types)
  /** no default has been set for forum channel */
    ENUMERATOR(DISCORD_FORUM_LAYOUT_NOT_SET, = 0)
  /** display posts as a list */
    ENUMERATOR(DISCORD_FORUM_LAYOUT_LIST_VIEW, = 1)
  /** display posts as a collection of tiles */
    ENUMERATOR_LAST(DISCORD_FORUM_LAYOUT_GALLERY_VIEW, = 1)
ENUM_END
#endif

/** @CCORD_pub_struct{discord_channel} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_channel)
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
STRUCT_END
#endif

/** @CCORD_pub_list{discord_channels} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_LIST(discord_channels)
    LISTTYPE_STRUCT(discord_channel)
LIST_END
#endif


#if GENCODECS_RECIPE == DATA
ENUM(discord_message_types)
    ENUMERATOR(DISCORD_MESSAGE_DEFAULT, = 0)
    ENUMERATOR(DISCORD_MESSAGE_RECIPIENT_ADD, = 1)
    ENUMERATOR(DISCORD_MESSAGE_RECIPIENT_REMOVE, = 2)
    ENUMERATOR(DISCORD_MESSAGE_CALL, = 3)
    ENUMERATOR(DISCORD_MESSAGE_CHANNEL_NAME_CHANGE, = 4)
    ENUMERATOR(DISCORD_MESSAGE_CHANNEL_ICON_CHANGE, = 5)
    ENUMERATOR(DISCORD_MESSAGE_CHANNEL_PINNED_MESSAGE, = 6)
    ENUMERATOR(DISCORD_MESSAGE_GUILD_MEMBER_JOIN, = 7)
    ENUMERATOR(DISCORD_MESSAGE_USER_PREMIUM_GUILD_SUBSCRIPTION, = 8)
    ENUMERATOR(DISCORD_MESSAGE_USER_PREMIUM_GUILD_SUBSCRIPTION_TIER_1, = 9)
    ENUMERATOR(DISCORD_MESSAGE_USER_PREMIUM_GUILD_SUBSCRIPTION_TIER_2, = 10)
    ENUMERATOR(DISCORD_MESSAGE_USER_PREMIUM_GUILD_SUBSCRIPTION_TIER_3, = 11)
    ENUMERATOR(DISCORD_MESSAGE_CHANNEL_FOLLOW_ADD, = 12)
    ENUMERATOR(DISCORD_MESSAGE_GUILD_DISCOVERY_DISQUALIFIED, = 14)
    ENUMERATOR(DISCORD_MESSAGE_GUILD_DISCOVERY_REQUALIFIED, = 15)
    ENUMERATOR(DISCORD_MESSAGE_GUILD_DISCOVERY_GRACE_PERIOD_INITIAL_WARNING, = 16)
    ENUMERATOR(DISCORD_MESSAGE_GUILD_DISCOVERY_GRACE_PERIOD_FINAL_WARNING, = 17)
    ENUMERATOR(DISCORD_MESSAGE_THREAD_CREATED, = 18)
    ENUMERATOR(DISCORD_MESSAGE_REPLY, = 19)
    ENUMERATOR(DISCORD_MESSAGE_CHAT_INPUT_COMMAND, = 20)
    ENUMERATOR(DISCORD_MESSAGE_THREAD_STARTER_MESSAGE, = 21)
    ENUMERATOR(DISCORD_MESSAGE_GUILD_INVITE_REMINDER, = 22)
    ENUMERATOR(DISCORD_MESSAGE_CONTEXT_MENU_COMMAND, = 23)
    ENUMERATOR(DISCORD_MESSAGE_AUTO_MODERATION_ACTION, = 24)
    ENUMERATOR(DISCORD_MESSAGE_ROLE_SUBSCRIPTION_PURCHASE, = 25)
    ENUMERATOR(DISCORD_MESSAGE_INTERACTION_PREMIUM_UPSELL, = 26)
    ENUMERATOR(DISCORD_MESSAGE_STAGE_START, = 27)
    ENUMERATOR(DISCORD_MESSAGE_STAGE_END, = 28)
    ENUMERATOR(DISCORD_MESSAGE_STAGE_SPEAKER, = 29)
    ENUMERATOR(DISCORD_MESSAGE_STAGE_TOPIC, = 31)
    ENUMERATOR_LAST(DISCORD_MESSAGE_GUILD_APPLICATION_PREMIUM_SUBSCRIPTION, = 32)
ENUM_END
#endif

#if GENCODECS_RECIPE == DATA
ENUM(discord_message_activity_types)
    ENUMERATOR(DISCORD_MESSAGE_ACTIVITY_JOIN, = 1)
    ENUMERATOR(DISCORD_MESSAGE_ACTIVITY_SPECTATE, = 2)
    ENUMERATOR(DISCORD_MESSAGE_ACTIVITY_LISTEN, = 3)
    ENUMERATOR_LAST(DISCORD_MESSAGE_ACTIVITY_JOIN_REQUEST, = 5)
ENUM_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_message_activity)
  /** type of message activity */
    FIELD_ENUM(type, discord_message_activity_types)
  /** party_id from a Rich Presence event */
    FIELD_PTR(party_id, char, *)
STRUCT_END
#endif

/** @defgroup DiscordAPIChannelMessageFlags Message flags
 * @ingroup DiscordAPIChannel
 *  @{ */

/** this message has been published to subscribed channels (via Channel
   Following) */
PP_DEFINE(DISCORD_MESSAGE_CROSSPOSTED 1 << 0)
/** this message originated from a message in another channel (via Channel
   Following) */
PP_DEFINE(DISCORD_MESSAGE_IS_CROSSPOST 1 << 1)
/** do not include any embed when serializing this message */
PP_DEFINE(DISCORD_MESSAGE_SUPPRESS_EMBEDS 1 << 2)
/** the source message for this crosspost has been deleted (via Channel
   Following) */
PP_DEFINE(DISCORD_MESSAGE_SOURCE_MESSAGE_DELETED 1 << 3)
/** this message came from the urgent message system */
PP_DEFINE(DISCORD_MESSAGE_URGENT 1 << 4)
/** this message has an associated thread, with the same ID as the message */
PP_DEFINE(DISCORD_MESSAGE_HAS_THREAD 1 << 5)
/** this message is only visible to the user who invoked the interaction */
PP_DEFINE(DISCORD_MESSAGE_EPHEMERAL 1 << 6)
/** this message is an interaction response and the bot is thinking */
PP_DEFINE(DISCORD_MESSAGE_LOADING 1 << 7)
/** this message failed to mention some roles and add the members to the
   thread */
PP_DEFINE(DISCORD_MESSAGE_FAILED_TO_MENTION_SOME_ROLES_IN_THREAD 1 << 8)
/** this message will not trigger push and desktop notifications */
PP_DEFINE(DISCORD_MESSAGE_SUPPRESS_NOTIFICATIONS 1 << 12)
/** this message is a voice message */
PP_DEFINE(DISCORD_MESSAGE_IS_VOICE_MESSAGE 1 << 13)

/** @} DiscordAPIChannelMessageFlags */

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_message_reference)
  /** id of the originating message */
    FIELD_SNOWFLAKE(message_id)
  /** id of the originating message's channel */
    FIELD_SNOWFLAKE(channel_id)
  /** id of the originating message's guild */
    FIELD_SNOWFLAKE(guild_id)
  /** when sending, whether to error if the referenced message doesn't exist
        instead of sending as normal (non-reply) message, default true */
    FIELD(fail_if_not_exists, bool, true)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_message} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_message)
  /** ID of the message */
    FIELD_SNOWFLAKE(id)
  /** ID of the channel the message was sent in */
    FIELD_SNOWFLAKE(channel_id)
  /** ID of the guild the message was sent in */
    FIELD_SNOWFLAKE(guild_id)
  /** the author of this message (not guaranteed to be a valid user) */
    FIELD_STRUCT_PTR(author, discord_user, *)
  /** partial guild member */
    FIELD_STRUCT_PTR(member, discord_guild_member, *)
  /** contents of the message */
    FIELD_PTR(content, char, *)
  /** when this message was sent */
    FIELD_TIMESTAMP(timestamp)
  /** when this message was edited (or null if never) */
  COND_WRITE(self->edited_timestamp != 0)
    FIELD_TIMESTAMP(edited_timestamp)
  COND_END
  /** whether this was a TTS message */
    FIELD(tts, bool, false)
  /** whether this message mentions everyone */
    FIELD(mention_everyone, bool, false)
  /** FIXME: expects additional member field */
  /** users specifically mentioned in the message */
    FIELD_STRUCT_PTR(mentions, discord_users, *)
  /** roles specifically mentioned in this message */
    FIELD_STRUCT_PTR(mention_roles, snowflakes, *)
  /** channels specifically mentioned in this message */
    FIELD_STRUCT_PTR(mention_channels, discord_channels, *)
  /** any attached files */
    FIELD_STRUCT_PTR(attachments, discord_attachments, *)
  /** any embedded content */
    FIELD_STRUCT_PTR(embeds, discord_embeds, *)
  /** reactions to the message */
    FIELD_STRUCT_PTR(reactions, discord_reactions, *)
  /** used for validating a message was sent */
    FIELD_PTR(nonce, json_char, *)
  /** whether this message is pinned */
    FIELD(pinned, bool, false)
  /** if the message is generated by a webhook, this is the webhook's ID */
    FIELD_SNOWFLAKE(webhook_id)
  /** type of message */
    FIELD_ENUM(type, discord_message_types)
  /** sent with Rich Presence-related chat embeds */
    FIELD_STRUCT_PTR(activity, discord_message_activity, *)
  /** sent with Rich Presence-related chat embeds */
    FIELD_STRUCT_PTR(application, discord_application, *)
  /** if the message is an interaction or an application-owned webhook, this
       is the ID of the application */
    FIELD_SNOWFLAKE(application_id)
  /** data showing the source of a crosspost, channel follow add, pin, or
       reply message */
    FIELD_STRUCT_PTR(message_reference, discord_message_reference, *)
  /** @ref DiscordAPIChannelMessageFlags combined as a bitfield */
    FIELD_BITMASK(flags)
  /** the message associated with the message_reference */
    FIELD_STRUCT_PTR(referenced_message, discord_message, *)
  /** sent if the message is a response to an interaction */
    FIELD_STRUCT_PTR(interaction, discord_message_interaction, *)
  /** the thread that was started from this message, includes
       @ref discord_thread_member */
    FIELD_STRUCT_PTR(thread, discord_channel, *)
  /** sent if the message contains components like buttons, action rows, or
       other interactive components */
    FIELD_STRUCT_PTR(components, discord_components, *)
  /** sent if the message contains stickers */
    FIELD_STRUCT_PTR(sticker_items, discord_sticker_items, *)
  /** @deprecated use `sticker_items` instead */
    FIELD_STRUCT_PTR(stickers, discord_stickers, *)
  /** A generally increasing integer that represents the approximate
        position of the message in a thread */
    FIELD(position, int, 0)
  /** data of the role subscription purchase or renewal that prompted
        this ROLE_SUBSCRIPTION_PURCHASE message */
    FIELD_STRUCT_PTR(role_subscription_data, discord_role_subscription_data, *)
  /** data for users, members, channels, and roles in the message's
        auto-populated select menus */
    FIELD_STRUCT_PTR(resolved, discord_resolved_data, *)
STRUCT_END
#endif

/** @CCORD_pub_list{discord_messages} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_LIST(discord_messages)
    LISTTYPE_STRUCT(discord_message)
LIST_END
#endif

/** @CCORD_pub_struct{discord_followed_channel} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_followed_channel)
  /** source channel id */
    FIELD_SNOWFLAKE(channel_id)
  /** created target webhook id */
    FIELD_SNOWFLAKE(webhook_id)
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_reaction_count_details)
  /** count of super reactions */
    FIELD(burst, int, 0)
  /** count of normal reactions */
    FIELD(normal, int, 0)
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_reaction)
  /** times this emoji has been used to react */
    FIELD(count, int, 0)
  /** breakdown of normal and super reaction counts for the associated emoji */
    FIELD_STRUCT_PTR(count_details, discord_reaction_count_details, *)
  /** whether the current user reacted using this emoji */
    FIELD(me, bool, false)
  /** whether the current user super-reacted using this emoji */
    FIELD(me_burst, bool, false)
  /** emoji information */
    FIELD_STRUCT_PTR(emoji, discord_emoji, *)
  /** HEX colors used for super reaction */
    FIELD_PTR(burst_colors, char, *)
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
LIST(discord_reactions)
    LISTTYPE_STRUCT(discord_reaction)
LIST_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_overwrite)
  /** role or user id */
    FIELD_SNOWFLAKE(id)
  /** either 0 (role) or 1 (member) */
    FIELD(type, int, 0)
  /** @ref DiscordPermissions bit set */
    FIELD_BITMASK(allow)
  /** @ref DiscordPermissions bit set */
    FIELD_BITMASK(deny)
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
LIST(discord_overwrites)
    LISTTYPE_STRUCT(discord_overwrite)
LIST_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_thread_metadata)
  /** whether the thread is archived */
    FIELD(archived, bool, false)
  /** duration in minutes to automatically archive the thread after recent
       activity, can be set to: 60, 1440, 3320, 10080 */
    FIELD(auto_archive_duration, int, 0)
  /** timestamp when the thread's archive status was last changed, used for
       calculating recent activity */
    FIELD_TIMESTAMP(archive_timestamp)
  /** whether the thread is locked; when a thread is locked only users with
       `MANAGE_THREADS` can unarchive it */
    FIELD(locked, bool, false)
  /** whether non-moderators cana dd other non-moderators to a thread; only
       available on private threads */
    FIELD(invitable, bool, false)
  /** timestamp when the thread was created; only populated for threads
       created after 2022-01-09 */
    FIELD_TIMESTAMP(create_timestamp)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_thread_member} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_thread_member)
  /** the id of the thread */
    FIELD_SNOWFLAKE(id)
  /** the id of the user */
    FIELD_SNOWFLAKE(user_id)
  /** the time the current user last joined the thread */
    FIELD_TIMESTAMP(join_timestamp)
  /** any user-thread settings, currently only used for notifications */
    FIELD_BITMASK(flags)
  /** additional information about the user */
    FIELD_STRUCT_PTR(member, discord_guild_member, *)
  /** the id of the guild @note used at `Thread Member Update` */
    FIELD_SNOWFLAKE(guild_id)
STRUCT_END
#endif

/** @CCORD_pub_list{discord_thread_members} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_LIST(discord_thread_members)
    LISTTYPE_STRUCT(discord_thread_member)
LIST_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_thread_default_reaction)
  /** 	the id of a guild's custom emoji */
    FIELD_SNOWFLAKE(emoji_id)
  /** the unicode character of the emoji */
    FIELD_PTR(emoji_name, char, *)
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_thread_tag)
  /** the id of the tag */
    FIELD_SNOWFLAKE(id)
  /** the name of the tag */
    FIELD_PTR(name, char, *)
  /** whether this tag can only be added to or removed from threads
        by a member with the MANAGE_THREADS permission */
    FIELD(moderated, bool, false)
  /** 	the id of a guild's custom emoji */
    FIELD_SNOWFLAKE(emoji_id)
  /** the unicode character of the emoji */
    FIELD_PTR(emoji_name, char, *)
STRUCT_END
#endif

/** @CCORD_pub_list{discord_thread_tags} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_LIST(discord_thread_tags)
    LISTTYPE_STRUCT(discord_thread_tag)
LIST_END
#endif


/** @CCORD_pub_struct{discord_embed_thumbnail} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_embed_thumbnail)
  /** source url of thumbnail (only supports http(s) and attachments) */
    FIELD_PTR(url, char, *)
  /** a proxied url of the thumbnail */
  COND_WRITE(self->proxy_url != NULL)
    FIELD_PTR(proxy_url, char, *)
  COND_END
  /** height of thumbnail */
  COND_WRITE(self->height != 0)
    FIELD(height, int, 0)
  COND_END
  /** width of thumbnail */
  COND_WRITE(self->width != 0)
    FIELD(width, int, 0)
  COND_END
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_embed_video} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_embed_video)
  /** source url of video */
  COND_WRITE(self->url != NULL)
    FIELD_PTR(url, char, *)
  COND_END
  /** a proxied url of the video */
  COND_WRITE(self->proxy_url != NULL)
    FIELD_PTR(proxy_url, char, *)
  COND_END
  /** height of video */
  COND_WRITE(self->height != 0)
    FIELD(height, int, 0)
  COND_END
  /** width of video */
  COND_WRITE(self->width != 0)
    FIELD(width, int, 0)
  COND_END
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_embed_image} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_embed_image)
  /** source url of image (only supports http(s) and attachments) */
    FIELD_PTR(url, char, *)
  /** a proxied url of the image */
  COND_WRITE(self->proxy_url != NULL)
    FIELD_PTR(proxy_url, char, *)
  COND_END
  /** height of image */
  COND_WRITE(self->height != 0)
    FIELD(height, int, 0)
  COND_END
  /** width of image */
  COND_WRITE(self->width != 0)
    FIELD(width, int, 0)
  COND_END
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_embed_provider} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_embed_provider)
  /** name of provider */
  COND_WRITE(self->name != NULL)
    FIELD_PTR(name, char, *)
  COND_END
  /** url of provider */
  COND_WRITE(self->url != NULL)
    FIELD_PTR(url, char, *)
  COND_END
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_embed_author} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_embed_author)
  /** name of author */
    FIELD_PTR(name, char, *)
  /** url of author */
  COND_WRITE(self->url != NULL)
    FIELD_PTR(url, char, *)
  COND_END
  /** url of author icon (only supports http(s) and attachments) */
  COND_WRITE(self->icon_url != NULL)
    FIELD_PTR(icon_url, char, *)
  COND_END
  /** a proxied url of author icon */
  COND_WRITE(self->proxy_icon_url != NULL)
    FIELD_PTR(proxy_icon_url, char, *)
  COND_END
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_embed_footer} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_embed_footer)
  /** footer text */
    FIELD_PTR(text, char, *)
  /** url of footer icon (only supports http(s) and attachments) */
  COND_WRITE(self->icon_url != NULL)
    FIELD_PTR(icon_url, char, *)
  COND_END
  /** a proxied url of footer icon */
  COND_WRITE(self->proxy_icon_url != NULL)
    FIELD_PTR(proxy_icon_url, char, *)
  COND_END
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_embed_field} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_embed_field)
  /** name of the field */
    FIELD_PTR(name, char, *)
  /** value of the field */
    FIELD_PTR(value, char, *)
  /** whether or not this field should display inline */
    FIELD_CUSTOM(Inline, "inline", bool, DECOR_BLANK, INIT_BLANK, CLEANUP_BLANK,
                 GENCODECS_JSON_ENCODER_bool, GENCODECS_JSON_DECODER_bool,
                 false)
STRUCT_END
#endif

/** @CCORD_pub_list{discord_embed_fields} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_LIST(discord_embed_fields)
    LISTTYPE_STRUCT(discord_embed_field)
LIST_END
#endif

/** @CCORD_pub_struct{discord_embed} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_embed)
  /** title of embed */
    FIELD_PTR(title, char, *)
  /** type of embed (always \"rich\" for webhook embeds) */
    FIELD_PTR(type, char, *)
  /** description of embed */
    FIELD_PTR(description, char, *)
  /** url of embed */
    FIELD_PTR(url, char, *)
  /** timestamp of embed content */
  COND_WRITE(self->timestamp != 0)
    FIELD_TIMESTAMP(timestamp)
  COND_END
  /** color code of the embed */
  FIELD(color, int, 0)
  /** footer information */
  COND_WRITE(self->footer != NULL)
    FIELD_STRUCT_PTR(footer, discord_embed_footer, *)
  COND_END
  /** image information */
  COND_WRITE(self->image != NULL)
    FIELD_STRUCT_PTR(image, discord_embed_image, *)
  COND_END
  /** thumbnail information */
  COND_WRITE(self->thumbnail != NULL)
    FIELD_STRUCT_PTR(thumbnail, discord_embed_thumbnail, *)
  COND_END
  /** video information */
  COND_WRITE(self->video != NULL)
    FIELD_STRUCT_PTR(video, discord_embed_video, *)
  COND_END
  COND_WRITE(self->provider != NULL)
    FIELD_STRUCT_PTR(provider, discord_embed_provider, *)
  COND_END
  COND_WRITE(self->author != NULL)
    FIELD_STRUCT_PTR(author, discord_embed_author, *)
  COND_END
  COND_WRITE(self->fields != NULL)
    FIELD_STRUCT_PTR(fields, discord_embed_fields, *)
  COND_END
STRUCT_END
#endif

/** @CCORD_pub_list{discord_embeds} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_LIST(discord_embeds)
    LISTTYPE_STRUCT(discord_embed)
LIST_END
#endif

/** @defgroup DiscordAPIChannelAttachmentFlags Attachment flags
 * @ingroup DiscordAPIChannel
 *  @{ */

/** this attachment has been edited using the remix feature on mobile */
PP_DEFINE(DISCORD_ATTACHMENT_IS_REMIX 1 << 2)

/** @} DiscordAPIChannelAttachmentFlags */

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_attachment)
  /** the file contents */
#if !(GENCODECS_RECIPE & JSON)
    FIELD_PTR(content, char, *)
#endif
  /** attachment ID */
  FIELD_SNOWFLAKE(id)
  /** name of file attached */
  FIELD_PTR(filename, char, *)
  /** description for the file */
  FIELD_PTR(description, char, *)
  /** the attachment media type */
  COND_WRITE(self->content_type != NULL)
    FIELD_PTR(content_type, char, *)
  COND_END
  /** size of file in bytes */
  FIELD(size, size_t, 0)
  /** source url of file */
  FIELD_PTR(url, char, *)
  /** proxied url of file */
  FIELD_PTR(proxy_url, char, *)
  /** height of file (if image) */
  COND_WRITE(self->height != 0)
    FIELD(height, int, 0)
  COND_END
  /** width of file (if image) */
  COND_WRITE(self->width != 0)
    FIELD(width, int, 0)
  COND_END
  /** whether this attachment is ephemeral */
  FIELD(ephemeral, bool, false)
  /** the duration of the audio file */
  COND_WRITE(self->duration_secs != 0)
    FIELD(duration_secs, int, 0)
  COND_END
  /** base64 encoded bytearray representing a sampled waveform */
  COND_WRITE(self->waveform != NULL)
    FIELD_PTR(waveform, char, *)
  COND_END
  /** attachment flags combined as a bitfield */
  COND_WRITE(self->flags != 0)
    FIELD_BITMASK(flags)
  COND_END
STRUCT_END
#endif

/** @CCORD_pub_list{discord_attachments} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_LIST(discord_attachments)
    LISTTYPE_STRUCT(discord_attachment)
LIST_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_channel_mention)
  /** ID of the channel */
    FIELD_SNOWFLAKE(id)
  /** ID of the guild containing the channel */
    FIELD_SNOWFLAKE(guild_id)
  /** the type of the channel */
    FIELD_ENUM(type, discord_channel_types)
  /** the name of the channel */
    FIELD_PTR(name, char, *)
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_allowed_mention)
  /** An array of allowed mention tpes to parse from the content */
    FIELD_STRUCT_PTR(parse, strings, *)
  /** Array of role_ids to mention (Max size of 100) */
    FIELD_STRUCT_PTR(roles, snowflakes, *)
  /** Array of user_ids to mention (Max size of 100) */
    FIELD_STRUCT_PTR(users, snowflakes, *)
  /** For replies, whether to mention the author of the message being
       replied to (default false) */
    FIELD(replied_user, bool, false)
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_role_subscription_data)
  /** 	the id of the sku and listing that the user is subscribed to */
    FIELD_SNOWFLAKE(role_subscription_listing_id)
  /** the name of the tier that the user is subscribed to */
    FIELD_PTR(tier_name, char, *)
  /** 	the cumulative number of months that the user has been subscribed for */
    FIELD(total_months_subscribed, int, 0)
  /** whether this notification is for a renewal rather than a new purchase */
    FIELD(is_renewal, bool, true)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_thread_response_body} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_thread_response_body)
  /** the archived threads */
    FIELD_STRUCT_PTR(threads, discord_channels, *)
  /** a thread member object for each returned thread the current user has
       joined */
    FIELD_STRUCT_PTR(members, discord_thread_members, *)
  /** whether there are potentially additional threads that could be returned
       on a subsequent call */
    FIELD(has_more, bool, false)
STRUCT_END
#endif

/*****************************************************************************
 * Channel REST parameters
 * **************************************************************************/

/** @CCORD_pub_struct{discord_modify_channel} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_modify_channel)
  /** @CCORD_reason{reason} */
#if GENCODECS_RECIPE == DATA
    FIELD_PTR(reason, char, *)
#endif
  /** 1-100 character channel name */
    FIELD_PTR(name, char, *)
  /* GROUP DM */
  /* TODO: implement base64 encoder */
#if 0
  /** base64 encoded icon */
    FIELD_PTR(icon, char, *)
#endif
  /* GUILD CHANNEL */
  /** the type of channel; only conversion between text and news is
       supported and only in guilds with the `NEWS` feature */
  COND_WRITE(self->type == DISCORD_CHANNEL_GUILD_TEXT || self->type == DISCORD_CHANNEL_GUILD_ANNOUNCEMENT)
    FIELD_ENUM(type, discord_channel_types)
  COND_END
  /** the position of the channel in the left-hand listing */
  COND_WRITE(self->position != 0)
    FIELD(position, int, 0)
  COND_END
  /** 0-1024 character channel topic */
  COND_WRITE(self->topic != NULL)
    FIELD_PTR(topic, char, *)
  COND_END
  /** whether the channel is nsfw */
  COND_WRITE(self->nsfw != false)
    FIELD(nsfw, bool, false)
  COND_END
  /** amount of seconds a user has to wait before sending another message
       (0-21600); bots, as well as users with the permission
       `MANAGE_MESSAGES` or `MANAGE_CHANNEL`, are unaffected */
  COND_WRITE(self->rate_limit_per_user != 0)
    FIELD(rate_limit_per_user, int, 0)
  COND_END
  /** the user limit of the voice channel; 0 refers to no limit, 1 to 99
       refers to a user limit */
  COND_WRITE(self->user_limit != 0)
    FIELD(user_limit, int, 0)
  COND_END
  /** channel or category-specific permissions */
  COND_WRITE(self->permission_overwrites != 0)
    FIELD_STRUCT_PTR(permission_overwrites, discord_overwrites, *)
  COND_END
  /** ID of the new parent category for a channel */
  COND_WRITE(self->parent_id != 0)
    FIELD_SNOWFLAKE(parent_id)
  COND_END
  /** channel voice region id, automatic when set to NULL */
  COND_WRITE(self->rtc_region != NULL)
    FIELD_PTR(rtc_region, char, *)
  COND_END
  /** the camera video quality mode of the voice channel */
  COND_WRITE(self->video_quality_mode != 0)
    FIELD(video_quality_mode, int, 0)
  COND_END
  /** the default duration that the clients use (not the API) for newly
       created threads in the channel, in minutes, to automatically archive
       the thread after recent activity */
  COND_WRITE(self->default_auto_archive_duration != 0)
    FIELD(default_auto_archive_duration, int, 0)
  COND_END
  /* THREAD */
  /** whether the thread is archived */
    FIELD(archived, bool, false)
  /** duration in minutes to automatically arhived the thread after recent
       activity, can be set to: 60, 1440, 4320, 10080 */
  COND_WRITE(self->auto_archive_duration != 0)
    FIELD(auto_archive_duration, int, 0)
  COND_END
  /** whether the thread is locked; when a thread is locked, only users with
       `MANAGE_THREADS` can unarchive it */
    FIELD(locked, bool, false)
  /** whether non-moderators can add other non-moderators to a thread; only
       available on private threads */
    FIELD(invitable, bool, false)
STRUCT_END
#endif

#if GENCODECS_RECIPE == DATA
STRUCT(discord_delete_channel)
  /** @CCORD_reason{reason} */
    FIELD_PTR(reason, char, *)
STRUCT_END
#endif

#if GENCODECS_RECIPE == DATA
STRUCT(discord_get_channel_messages)
  /** get messages around this message ID */
  COND_WRITE(self->around != 0)
    FIELD_SNOWFLAKE(around)
  COND_END
  /** get messages before this message ID */
  COND_WRITE(self->before != 0)
    FIELD_SNOWFLAKE(before)
  COND_END
  /** get messages after this message ID */
  COND_WRITE(self->after != 0)
    FIELD_SNOWFLAKE(after)
  COND_END
  /** max number of messages to return (1-100) */
  COND_WRITE(self->limit != 0)
    FIELD(limit, int, 50)
  COND_END
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_create_message} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_create_message)
  /** the message contents (up to 2000 characters) */
    FIELD_PTR(content, char, *)
  /** true if this is a TTS message */
    FIELD(tts, bool, false)
  /** embedded `rich` content (up to 6000 characters) */
    FIELD_STRUCT_PTR(embeds, discord_embeds, *)
  /** allowed mentions for the message */
  COND_WRITE(self->allowed_mentions != NULL)
    FIELD_STRUCT_PTR(allowed_mentions, discord_allowed_mention, *)
  COND_END
  /** include to make your message a reply */
  COND_WRITE(self->message_reference != NULL)
    FIELD_STRUCT_PTR(message_reference, discord_message_reference, *)
  COND_END
  /** the components to include with the message */
  COND_WRITE(self->components != NULL)
    FIELD_STRUCT_PTR(components, discord_components, *)
  COND_END
  /** IDs of up to 3 stickers in the server to send in the message */
  COND_WRITE(self->sticker_ids != NULL)
    FIELD_STRUCT_PTR(sticker_ids, snowflakes, *)
  COND_END
  /** attachment objects with filename and description */
  COND_WRITE(self->attachments != NULL)
    FIELD_STRUCT_PTR(attachments, discord_attachments, *)
  COND_END
  /** @ref DiscordAPIChannelMessageFlags combined as a bitfield (only 
       `SUPPRESS_EMBEDS` can be set) */
  COND_WRITE(self->flags != 0)
    FIELD_BITMASK(flags)
  COND_END
STRUCT_END
#endif

#if GENCODECS_RECIPE == DATA
STRUCT(discord_get_reactions)
  /** get users after this user ID */
  COND_WRITE(self->after != 0)
    FIELD_SNOWFLAKE(after)
  COND_END
  /** max number of users to return (1-100) */
  COND_WRITE(self->limit != 0)
    FIELD(limit, int, 0)
  COND_END
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_edit_message} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_edit_message)
  /** the message contents (up to 2000 characters) */
    FIELD_PTR(content, char, *)
  /** embedded `rich` content (up to 6000 characters) */
    FIELD_STRUCT_PTR(embeds, discord_embeds, *)
  /** @ref DiscordAPIChannelMessageFlags combined as a bitfield (only
       `SUPPRESS_EMBEDS` can be set) */
  COND_WRITE(self->flags != 0)
    FIELD_BITMASK(flags)
  COND_END
  /** allowed mentions for the message */
  COND_WRITE(self->allowed_mentions != NULL)
    FIELD_STRUCT_PTR(allowed_mentions, discord_allowed_mention, *)
  COND_END
  /** the components to include with the message */
  COND_WRITE(self->components != NULL)
    FIELD_STRUCT_PTR(components, discord_components, *)
  COND_END
  /** attachment objects with filename and description */
  COND_WRITE(self->attachments != NULL)
    FIELD_STRUCT_PTR(attachments, discord_attachments, *)
  COND_END
STRUCT_END
#endif

#if GENCODECS_RECIPE == DATA
STRUCT(discord_delete_message)
  /** @CCORD_reason{reason} */
    FIELD_PTR(reason, char, *)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_bulk_delete_messages} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_bulk_delete_messages)
  /** @CCORD_reason{reason} */
#if GENCODECS_RECIPE == DATA
    FIELD_PTR(reason, char, *)
#endif
  /** an array of message ids to delete (2-100) */
    FIELD_STRUCT_PTR(messages, snowflakes, *)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_edit_channel_permissions} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_edit_channel_permissions)
  /** @CCORD_reason{reason} */
#if GENCODECS_RECIPE == DATA
    FIELD_PTR(reason, char, *)
#endif
  /** the bitwise value of all allowed permissions (default \"0\")
        @see @ref DiscordPermissions */
  COND_WRITE(self->allow != 0)
    FIELD_BITMASK(allow)
  COND_END
  /** the bitwise value of all disallowed permissions (default \"0\") 
        @see @ref DiscordPermissions */
  COND_WRITE(self->deny != 0)
    FIELD_BITMASK(deny)
  COND_END
  /** 0 for a role or 1 for a member */
    FIELD(type, int, 0)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_create_channel_invite} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_create_channel_invite)
  /** @CCORD_reason{reason} */
#if GENCODECS_RECIPE == DATA
    FIELD_PTR(reason, char, *)
#endif
  /** duration of invite in seconds before expiry, or 0 for never. between
       0 and 604800 (7 days) */
  COND_WRITE(self->max_age != 0)
    FIELD(max_age, int, 86400)
  COND_END
  /** max number of uses or 0 for unlimited. betwee 0 and 100 */
  COND_WRITE(self->max_uses != 0)
    FIELD(max_uses, int, 0)
  COND_END
  /** whether this invite only grants temporary membership */
  COND_WRITE(self->temporary != 0)
    FIELD(temporary, bool, false)
  COND_END
  /** if true, don't true to reuse a similar invite (useful for creating
       many unique one time use invites) */
  COND_WRITE(self->unique != 0)
    FIELD(unique, bool, false)
  COND_END
  /** the type of target for this voice channel invite */
  COND_WRITE(self->target_type != 0)
    FIELD_ENUM(target_type, discord_invite_target_types)
  COND_END
  /** the id of the user whose stream to display for this invite, required
       if `target_type` is 1, the user must be streaming in the channel */
  COND_WRITE(self->target_user_id != 0)
    FIELD_SNOWFLAKE(target_user_id)
  COND_END
  /** the id of the embedded application to open for this invite, required
       if `target_type` is 2, the application must have the `EMBEDDED` flag */
  COND_WRITE(self->target_application_id != 0)
    FIELD_SNOWFLAKE(target_application_id)
  COND_END
STRUCT_END
#endif

#if GENCODECS_RECIPE == DATA
STRUCT(discord_delete_channel_permission)
  /** @CCORD_reason{reason} */
    FIELD_PTR(reason, char, *)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_follow_news_channel} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_follow_news_channel)
  /** id of target channel */
  COND_WRITE(self->webhook_channel_id != 0)
    FIELD_SNOWFLAKE(webhook_channel_id)
  COND_END
STRUCT_END
#endif

#if GENCODECS_RECIPE == DATA
STRUCT(discord_pin_message)
  /** @CCORD_reason{reason} */
    FIELD_PTR(reason, char, *)
STRUCT_END
#endif

#if GENCODECS_RECIPE == DATA
STRUCT(discord_unpin_message)
  /** @CCORD_reason{reason} */
    FIELD_PTR(reason, char, *)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_group_dm_add_recipient} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_group_dm_add_recipient)
  /** access token of a user that has granted your app the `gdm.join` scope */
    FIELD_PTR(access_token, char, *)
  /** nickname of the user being added */
    FIELD_PTR(nick, char, *)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_start_thread_with_message} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_start_thread_with_message)
  /** @CCORD_reason{reason} */
#if GENCODECS_RECIPE == DATA
    FIELD_PTR(reason, char, *)
#endif
  /** 1-100 character channel name */
    FIELD_PTR(name, char, *)
  /** duration in minutes to automatically archive the thread after recent
       activity, can be set to: 60, 1440, 4320, 10080 */
  COND_WRITE(self->auto_archive_duration != 0)
    FIELD(auto_archive_duration, int, 0)
  COND_END
  /** amount of seconds a user has to wait before sending another message
       (0-21600) */
  COND_WRITE(self->rate_limit_per_user >= 0
             && self->rate_limit_per_user <= 21600)
    FIELD(rate_limit_per_user, int, 0)
  COND_END
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_start_thread_without_message} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_start_thread_without_message)
  /** @CCORD_reason{reason} */
#if GENCODECS_RECIPE == DATA
    FIELD_PTR(reason, char, *)
#endif
  /** 1-100 character channel name */
    FIELD_PTR(name, char, *)
  /** duration in minutes to automatically archive the thread after recent
       activity, can be set to: 60, 1440, 4320, 10080 */
  COND_WRITE(self->auto_archive_duration != 0)
    FIELD(auto_archive_duration, int, 0)
  COND_END
  /** the type of thread to create */
    FIELD_ENUM(type, discord_channel_types)
  /** whether non-moderators can add other non-moderators to a thread; only
       available when creating a private thread */
    FIELD(invitable, bool, false)
  /** amount of seconds a user has to wait before sending another message
       (0-21600) */
  COND_WRITE(self->rate_limit_per_user >= 0
             && self->rate_limit_per_user <= 21600)
    FIELD(rate_limit_per_user, int, 0)
  COND_END
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_list_active_threads} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_list_active_threads)
  /** the active threads */
  COND_WRITE(self->threads != NULL)
    FIELD_STRUCT_PTR(threads, discord_channels, *)
  COND_END
  /** a thread member object for each returned thread the current user has
       joined */
  COND_WRITE(self->members != NULL)
    FIELD_STRUCT_PTR(members, discord_thread_members, *)
  COND_END
  /** whether there are potentially additional threads that could be returned
       on a subsequent call */
    FIELD(has_more, bool, false)
STRUCT_END
#endif
