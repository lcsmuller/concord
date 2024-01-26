/*****************************************************************************
 * Stage Instance Datatypes
 * **************************************************************************/

#if GENCODECS_RECIPE == DATA
ENUM(discord_privacy_level)
  /** the stage instance is visible publicly @deprecated deprecated value */
    ENUMERATOR(DISCORD_PRIVACY_PUBLIC, = 1)
  /** the stage instance is visible to only guild members */
    ENUMERATOR_LAST(DISCORD_PRIVACY_GUILD_ONLY, = 2)
ENUM_END
#endif

/** @CCORD_pub_struct{discord_stage_instance} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_stage_instance)
  /** the ID of this stage instance */
    FIELD_SNOWFLAKE(id)
  /** the guild ID of the associated stage channel */
    FIELD_SNOWFLAKE(guild_id)
  /** the ID of the associated stage channel */
    FIELD_SNOWFLAKE(channel_id)
  /** the topic of the Stage instance (1-120 characters) */
    FIELD_PTR(topic, char, *)
  /** the privacy level of the stage instance */
    FIELD_ENUM(privacy_level, discord_privacy_level)
  /** whether or not stage discovery is disabled @deprecated deprecated field */
    FIELD(discoverable_disabled, bool, false)
  /** the id of the scheduled event for this Stage instance */
    FIELD_SNOWFLAKE(guild_scheduled_event_id)
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
LIST(discord_stage_instances)
    LISTTYPE_STRUCT(discord_stage_instance)
LIST_END
#endif

/*****************************************************************************
 * Stage Instance REST parameters
 * **************************************************************************/

/** @CCORD_pub_struct{discord_create_stage_instance} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_create_stage_instance)
  /** @CCORD_reason{reason} */
#if GENCODECS_RECIPE == DATA
    FIELD_PTR(reason, char, *)
#endif
  /** the ID of the stage channel */
    FIELD_SNOWFLAKE(channel_id)
  /** the topic of the Stage instance (1-120 characters) */
    FIELD_PTR(topic, char, *)
  /** the privacy level of the stage instance */
  COND_WRITE(self->privacy_level != 0)
    FIELD_ENUM(privacy_level, discord_privacy_level)
  COND_END
  /** notify @everyone that a Stage instance has started */
  COND_WRITE(self->send_start_notification != false)
    FIELD(send_start_notification, bool, false)
  COND_END
  /** the id of the scheduled event for this Stage instance */
  COND_WRITE(self->guild_scheduled_event_id != 0)
    FIELD_SNOWFLAKE(guild_scheduled_event_id)
  COND_END
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_modify_stage_instance} */
#if GENCODECS_RECIPE & (DATA | JSON_DECODER)
PUB_STRUCT(discord_modify_stage_instance)
  /** @CCORD_reason{reason} */
#if GENCODECS_RECIPE == DATA
    FIELD_PTR(reason, char, *)
#endif
  /** the topic of the Stage instance (1-120 characters) */
  COND_WRITE(self->topic != NULL)
    FIELD_PTR(topic, char, *)
  COND_END
  /** the privacy level of the stage instance */
  COND_WRITE(self->privacy_level != 0)
    FIELD_ENUM(privacy_level, discord_privacy_level)
  COND_END
STRUCT_END
#endif

#if GENCODECS_RECIPE == DATA
STRUCT(discord_delete_stage_instance)
  /** @CCORD_reason{reason} */
    FIELD_PTR(reason, char, *)
STRUCT_END
#endif
