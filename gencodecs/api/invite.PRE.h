/*****************************************************************************
 * Invite Datatypes
 * **************************************************************************/

#if GENCODECS_RECIPE == DATA
ENUM(discord_invite_target_types)
    ENUMERATOR(DISCORD_INVITE_TARGET_STREAM, = 1)
    ENUMERATOR_LAST(DISCORD_INVITE_TARGET_EMBEDDED_APPLICATION, = 2)
ENUM_END
#endif

/** @CCORD_pub_struct{discord_invite} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_invite)
  /** the invite code (unique ID) */
    FIELD_PTR(code, char, *)
  /** the guild this invite is for */
  COND_WRITE(self->guild != NULL)
    FIELD_STRUCT_PTR(guild, discord_guild, *)
  COND_END
  /** the channel this invite is for */
    FIELD_STRUCT_PTR(channel, discord_channel, *)
  /** the user who created the invite */
  COND_WRITE(self->inviter != NULL)
    FIELD_STRUCT_PTR(inviter, discord_user, *)
  COND_END
  /** the type of target for this voice channel invite */
  COND_WRITE(self->target_type != 0)
    FIELD_ENUM(target_type, discord_invite_target_types)
  COND_END
  /** the user whose stream to display for this voice channel stream invite */
  COND_WRITE(self->target_user != NULL)
    FIELD_STRUCT_PTR(target_user, discord_user, *)
  COND_END
  /** the embedded application to open for this voice channel embedded
       application invite */
  COND_WRITE(self->target_application != NULL)
    FIELD_STRUCT_PTR(target_application, discord_application, *)
  COND_END
  /** approximate count of online members */
  COND_WRITE(self->approximate_presence_count != 0)
    FIELD(approximate_presence_count, int, 0)
  COND_END
  /** approximate count of total members */
  COND_WRITE(self->approximate_member_count != 0)
    FIELD(approximate_member_count, int, 0)
  COND_END
  /** the expiration date of this invite */
  COND_WRITE(self->expires_at != 0)
    FIELD_TIMESTAMP(expires_at)
  COND_END
  /** stage instance data if there is a public stage instance in the stage
       channel this invite is for */
  COND_WRITE(self->stage_instance != NULL)
    FIELD_STRUCT_PTR(stage_instance, discord_invite_stage_instance, *)
  COND_END
  /** guild scheduled event data, only included if `guild_scheduled_event_id`
       contains a valid guild scheduled event ID */
  COND_WRITE(self->guild_scheduled_event != NULL)
    FIELD_STRUCT_PTR(guild_scheduled_event, discord_guild_scheduled_event, *)
  COND_END
STRUCT_END
#endif

/** @CCORD_pub_list{discord_invites} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_LIST(discord_invites)
    LISTTYPE_STRUCT(discord_invite)
LIST_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_invite_metadata)
  /** number of times this invite has been used */
    FIELD(uses, int, 0)
  /** max number of times this invite can been used */
    FIELD(max_uses, int, 0)
  /** duration (in seconds) after which the invite expires */
    FIELD(max_age, int, 0)
  /** whether this invite only grants temporary membership */
    FIELD(temporary, bool, false)
  /** when this invite was created */
    FIELD_TIMESTAMP(created_at)
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_invite_stage_instance)
  /** the members speaking in the Stage */
    FIELD_STRUCT_PTR(members, discord_guild_members, *)
  /** the number of users in the Stage */
    FIELD(participant_count, int, 0)
  /** the number of users speaking in the Stage */
    FIELD(speaker_count, int, 0)
  /** the topic of the Stage instance (1-120 characters) */
    FIELD_PTR(topic, char, *)
STRUCT_END
#endif

/*****************************************************************************
 * Invite REST parameters
 * **************************************************************************/

/** @CCORD_pub_struct{discord_get_invite} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_get_invite)
  /** whether the invite should contain approximate member counts */
  COND_WRITE(self->with_counts != false)
    FIELD(with_counts, bool, false)
  COND_END
  /** whether the invite should contain the expiration date */
  COND_WRITE(self->with_expiration != false)
    FIELD(with_expiration, bool, false)
  COND_END
  /** the guild scheduled event to include with the invite */
  COND_WRITE(self->guild_scheduled_event_id != 0)
    FIELD_SNOWFLAKE(guild_scheduled_event_id)
  COND_END
STRUCT_END
#endif

#if GENCODECS_RECIPE == DATA
STRUCT(discord_delete_invite)
  /** @CCORD_reason{reason} */
    FIELD_PTR(reason, char, *)
STRUCT_END
#endif
