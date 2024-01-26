/*****************************************************************************
 * Emoji Datatypes
 * **************************************************************************/

/** @CCORD_pub_struct{discord_emoji} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_emoji)
  /** emoji ID */
    FIELD_SNOWFLAKE(id)
  /** emoji name */
    FIELD_PTR(name, char, *)
  /** roles allowed to use this emoji */
  COND_WRITE(self->roles != NULL)
    FIELD_STRUCT_PTR(roles, snowflakes, *)
  COND_END
  /** user that created this emoji */
  COND_WRITE(self->user != NULL)
    FIELD_STRUCT_PTR(user, discord_user, *)
  COND_END
  /** whether this emoji must be wrapped in colons */
  COND_WRITE(self->require_colons != false)
    FIELD(require_colons, bool, false)
  COND_END
  /** whether this emoji is managed */
  COND_WRITE(self->managed != false)
    FIELD(managed, bool, false)
  COND_END
  /** whether this emoji is animated */
  COND_WRITE(self->animated != false)
    FIELD(animated, bool, false)
  COND_END
  /** whether this emoji can be used, may be false due to loss of Server
       Boosts */
  COND_WRITE(self->available != false)
    FIELD(available, bool, false)
  COND_END
STRUCT_END
#endif

/** @CCORD_pub_list{discord_emojis} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_LIST(discord_emojis)
    LISTTYPE_STRUCT(discord_emoji)
LIST_END
#endif

/*****************************************************************************
 * Emoji REST parameters
 * **************************************************************************/

/** @CCORD_pub_struct{discord_create_guild_emoji} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_create_guild_emoji)
  /** @CCORD_reason{reason} */
#if GENCODECS_RECIPE == DATA
    FIELD_PTR(reason, char, *)
#endif
  /** name of the emoji */
    FIELD_PTR(name, char, *)
  /* TODO: implement base64 encoding */
  /** the 128x128 emoji image */
    FIELD_PTR(image, char, *)
  /** roles allowed to use this emoji */
    FIELD_STRUCT_PTR(roles, snowflakes, *)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_modify_guild_emoji} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_modify_guild_emoji)
  /** @CCORD_reason{reason} */
#if GENCODECS_RECIPE == DATA
    FIELD_PTR(reason, char, *)
#endif
  /** name of the emoji */
    FIELD_PTR(name, char, *)
  /** roles allowed to use this emoji */
  COND_WRITE(self->roles != NULL)
    FIELD_STRUCT_PTR(roles, snowflakes, *)
  COND_END
STRUCT_END
#endif

#if GENCODECS_RECIPE == DATA
STRUCT(discord_delete_guild_emoji)
  /** @CCORD_reason{reason} */
    FIELD_PTR(reason, char, *)
STRUCT_END
#endif
