/*****************************************************************************
 * Guild Scheduled Event Datatypes
 * **************************************************************************/

#if GENCODECS_RECIPE == DATA
ENUM(discord_guild_scheduled_event_privacy_level)
  /** the scheduled event is only accessible to guild members */
    ENUMERATOR_LAST(DISCORD_GUILD_SCHEDULED_EVENT_GUILD_ONLY, = 2)
ENUM_END
#endif

#if GENCODECS_RECIPE == DATA
ENUM(discord_guild_scheduled_event_entity_types)
    ENUMERATOR(DISCORD_GUILD_SCHEDULED_EVENT_ENTITY_STAGE_INSTANCE, = 1)
    ENUMERATOR(DISCORD_GUILD_SCHEDULED_EVENT_ENTITY_VOICE, = 2)
    ENUMERATOR_LAST(DISCORD_GUILD_SCHEDULED_EVENT_ENTITY_EXTERNAL, = 3)
ENUM_END
#endif

#if GENCODECS_RECIPE == DATA
ENUM(discord_guild_scheduled_event_status)
    ENUMERATOR(DISCORD_GUILD_SCHEDULED_EVENT_SCHEDULED, = 1)
    ENUMERATOR(DISCORD_GUILD_SCHEDULED_EVENT_ACTIVE, = 2)
    ENUMERATOR(DISCORD_GUILD_SCHEDULED_EVENT_COMPLETED, = 3)
    ENUMERATOR_LAST(DISCORD_GUILD_SCHEDULED_EVENT_CANCELED, = 4)
ENUM_END
#endif

/** @CCORD_pub_struct{discord_guild_scheduled_event} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_guild_scheduled_event)
  /** the ID of the scheduled event */
    FIELD_SNOWFLAKE(id)
  /** the guild ID which the scheduled event belongs to */
    FIELD_SNOWFLAKE(guild_id)
  /** the channel ID in which the scheduled event will be hosted, or `NULL`
       if `entity_type` is @ref DISCORD_SCHEDULED_ENTITY_EXTERNAL */
    FIELD_SNOWFLAKE(channel_id)
  /** the ID of the user that created the scheduled event */
    FIELD_SNOWFLAKE(creator_id)
  /** the name of the scheduled event (1-100 characters) */
    FIELD_PTR(name, char, *)
  /** the description of the scheduled event (1-1000 characters) */
    FIELD_PTR(description, char, *)
  /** the time the scheduled event will start */
    FIELD_TIMESTAMP(scheduled_start_time)
  /** the time the scheduled event will end, required if `entity_type` is
       @ref DISCORD_SCHEDULED_ENTITY_EXTERNAL */
    FIELD_TIMESTAMP(scheduled_end_time)
  /** the privacy level of the scheduled event */
  COND_WRITE(self->privacy_level != 0)
    FIELD_ENUM(privacy_level, discord_guild_scheduled_event_privacy_level)
  COND_END
  /** the status of the scheduled event */
  COND_WRITE(self->status != 0)
    FIELD_ENUM(status, discord_guild_scheduled_event_status)
  COND_END
  /** the type of scheduled event */
  COND_WRITE(self->entity_type != 0)
    FIELD_ENUM(entity_type, discord_guild_scheduled_event_entity_types)
  COND_END
  /** the ID of an entity associated with a guild scheduled event */
    FIELD_SNOWFLAKE(entity_id)
  /** additional metadata for the guild scheduled event */
  COND_WRITE(self->entity_metadata != NULL)
    FIELD_STRUCT_PTR(entity_metadata, discord_guild_scheduled_event_entity_metadata, *)
  COND_END
  /** the user that created the scheduled event */
  COND_WRITE(self->creator != NULL)
    FIELD_STRUCT_PTR(creator, discord_user, *)
  COND_END
  /** the number of users subscribed to the scheduled event */
    FIELD(user_count, int, 0)
  /** the cover image hashof the scheduled event */
    FIELD_PTR(image, char, *)
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_guild_scheduled_event_entity_metadata)
  /** location of the event (1-100 characters) */
  COND_WRITE(self->location != NULL)
    FIELD_PTR(location, char, *)
  COND_END
STRUCT_END
#endif

/** @CCORD_pub_list{discord_guild_scheduled_events} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_LIST(discord_guild_scheduled_events)
    LISTTYPE_STRUCT(discord_guild_scheduled_event)
LIST_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_guild_scheduled_event_user)
  /** the scheduled event ID which the user subscribed to */
    FIELD_SNOWFLAKE(guild_scheduled_event_id)
  /** user which subscribed to an event */
    FIELD_STRUCT_PTR(user, discord_user, *)
  /** guild member data for this user for the guild which this event belongs
       to, if any */
  COND_WRITE(self->member != NULL)
    FIELD_STRUCT_PTR(member, discord_guild_member, *)
  COND_END
STRUCT_END
#endif

/** @CCORD_pub_list{discord_guild_scheduled_event_users} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_LIST(discord_guild_scheduled_event_users)
    LISTTYPE_STRUCT(discord_guild_scheduled_event_user)
LIST_END
#endif

/*****************************************************************************
 * Guild Scheduled Event REST parameters
 * **************************************************************************/

#if GENCODECS_RECIPE == DATA
STRUCT(discord_list_guild_scheduled_events)
  /** include number of users subscribed to each event */
    FIELD(with_user_count, bool, false)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_create_guild_scheduled_event} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_create_guild_scheduled_event)
  /** @CCORD_reason{reason} */
#if GENCODECS_RECIPE == DATA
    FIELD_PTR(reason, char, *)
#endif
  /** the channel ID of the scheduled event */
  COND_WRITE(self->channel_id != 0)
    FIELD_SNOWFLAKE(channel_id)
  COND_END
  /** the entity metadata of the scheduled event */
  COND_WRITE(self->entity_metadata != NULL)
    FIELD_STRUCT_PTR(entity_metadata, discord_guild_scheduled_event_entity_metadata, *)
  COND_END
  /** the name of the scheduled event */
    FIELD_PTR(name, char, *)
  /** the privacy level of the scheduled event */
    FIELD_ENUM(privacy_level, discord_guild_scheduled_event_privacy_level)
  /** the time the scheduled event will start */
    FIELD_TIMESTAMP(scheduled_start_time)
  /** the time the scheduled event will end */
  COND_WRITE(self->scheduled_end_time != 0)
    FIELD_TIMESTAMP(scheduled_end_time)
  COND_END
  /** the description of the scheduled event */
  COND_WRITE(self->description != NULL)
    FIELD_PTR(description, char, *)
  COND_END
  /** the entity type of the scheduled event */
    FIELD_ENUM(entity_type, discord_guild_scheduled_event_entity_types)
  /** the cover image of the scheduled event */
    FIELD_PTR(image, char, *)
STRUCT_END
#endif

#if GENCODECS_RECIPE == DATA
STRUCT(discord_get_guild_scheduled_event)
  /** include number of users subscribed to each event */
    FIELD(with_user_count, bool, false)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_modify_guild_scheduled_event} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_modify_guild_scheduled_event)
  /** @CCORD_reason{reason} */
#if GENCODECS_RECIPE == DATA
    FIELD_PTR(reason, char, *)
#endif
  /** the channel ID of the scheduled event */
  COND_WRITE(self->channel_id != 0)
    FIELD_SNOWFLAKE(channel_id)
  COND_END
  /** the entity metadata of the scheduled event */
  COND_WRITE(self->entity_metadata != NULL)
    FIELD_STRUCT_PTR(entity_metadata, discord_guild_scheduled_event_entity_metadata, *)
  COND_END
  /** the name of the scheduled event */
  COND_WRITE(self->name != NULL)
    FIELD_PTR(name, char, *)
  COND_END
  /** the privacy level of the scheduled event */
  COND_WRITE(self->privacy_level != 0)
    FIELD_ENUM(privacy_level, discord_guild_scheduled_event_privacy_level)
  COND_END
  /** the time the scheduled event will start */
  COND_WRITE(self->scheduled_start_time != 0)
    FIELD_TIMESTAMP(scheduled_start_time)
  COND_END
  /** the time the scheduled event will end */
  COND_WRITE(self->scheduled_end_time != 0)
    FIELD_TIMESTAMP(scheduled_end_time)
  COND_END
  /** the description of the scheduled event */
  COND_WRITE(self->description != NULL)
    FIELD_PTR(description, char, *)
  COND_END
  /** the entity type of the scheduled event */
  COND_WRITE(self->entity_type != 0)
    FIELD_ENUM(entity_type, discord_guild_scheduled_event_entity_types)
  COND_END
  /** the status of the scheduled event */
  COND_WRITE(self->status != 0)
    FIELD_ENUM(status, discord_guild_scheduled_event_status)
  COND_END
  /** the cover image of the scheduled event */
  COND_WRITE(self->image != NULL)
    FIELD_PTR(image, char, *)
  COND_END
STRUCT_END
#endif

#if GENCODECS_RECIPE == DATA
STRUCT(discord_get_guild_scheduled_event_users)
  /** number of users to return (up to maximum of 100) */
  COND_WRITE(self->limit != 0)
    FIELD(limit, int, 0)
  COND_END
  /** include guild member data if exists */
  COND_WRITE(self->with_member != false)
    FIELD(with_member, bool, false)
  COND_END
  /** consider only users before given user ID */
  COND_WRITE(self->before != 0)
    FIELD_SNOWFLAKE(before)
  COND_END
  /** consider only users after given user ID */
  COND_WRITE(self->after != 0)
    FIELD_SNOWFLAKE(after)
  COND_END
STRUCT_END
#endif
