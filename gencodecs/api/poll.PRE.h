/*****************************************************************************
 * Pool Datatypes
 * **************************************************************************/

/** Discord polls
 * @see https://discord.com/developers/docs/resources/poll */

/** @CCORD_pub_struct{discord_poll_question} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_poll_question)
  /** the text of the field */
  COND_WRITE(self->text != NULL)
    FIELD_PTR(text, char, *)
  COND_END
  /** the emoji of the field */
  COND_WRITE(self->emoji != NULL)
    FIELD_STRUCT_PTR(emoji, discord_emoji, *)
  COND_END
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_poll} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_poll)
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

/** @CCORD_pub_list{discord_poll_answer_voters} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_poll_answer_voters)
  /** users who voted for this answer */
    FIELD_STRUCT_PTR(users, discord_user, *)
STRUCT_END
#endif
