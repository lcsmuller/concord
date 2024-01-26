/*****************************************************************************
 * Auto Moderation Datatypes
 * **************************************************************************/

/** @brief Characterizes the type of content which can trigger the rule */
#if GENCODECS_RECIPE == DATA
ENUM(discord_auto_moderation_trigger_types)
  /**
   * check if content contains words from a user defined list of keywords
   * @note maximum 6 per guild
   */
    ENUMERATOR(DISCORD_AUTO_MODERATION_KEYWORD, = 1)
  /**
   * check if content represents generic spam
   * @note maximum 1 per guild
   */
    ENUMERATOR(DISCORD_AUTO_MODERATION_SPAM, = 3)
  /**
   * check if content contains words from internal pre-defined wordsets
   * @note maximum 1 per guild
   */
    ENUMERATOR(DISCORD_AUTO_MODERATION_KEYWORD_PRESET, = 4)
  /**
   * check if content contains more unique mentions than allowed
   * @note maximum 1 per guild
   */
    ENUMERATOR_LAST(DISCORD_AUTO_MODERATION_MENTION_SPAM, = 5)
ENUM_END
#endif

#if GENCODECS_RECIPE == DATA
ENUM(discord_auto_moderation_keyword_preset_types)
    /** words that may be considered forms of swearing or cursing */
    ENUMERATOR(DISCORD_AUTO_MODERATION_PROFANITY, = 1)
    /** words that refer to sexually explicit behavior or activity */
    ENUMERATOR(DISCORD_AUTO_MODERATION_SEXUAL_CONTENT, = 2)
    /** personal insults or words that may be considered hate speech */
    ENUMERATOR_LAST(DISCORD_AUTO_MODERATION_SLURS, = 3)
ENUM_END
#endif

#if GENCODECS_RECIPE == DATA
ENUM(discord_auto_moderation_event_types)
    /** when a member sends or edits a message in the guild */
    ENUMERATOR_LAST(DISCORD_AUTO_MODERATION_MESSAGE_SEND, = 1)
ENUM_END
#endif

#if GENCODECS_RECIPE == DATA
ENUM(discord_auto_moderation_action_types)
    /** blocks the content of a message according to the rule */
    ENUMERATOR(DISCORD_AUTO_MODERATION_ACTION_BLOCK_MESSAGE, = 1)
    /** logs user content to a specified channel */
    ENUMERATOR(DISCORD_AUTO_MODERATION_ACTION_SEND_ALERT_MESSAGE, = 2)
    /** timeout user for a specified duration */
    ENUMERATOR_LAST(DISCORD_AUTO_MODERATION_ACTION_TIMEOUT, = 3)
ENUM_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_auto_moderation_trigger_metadata)
  /**
   * substrings which will be searched for in content
   * @note associated with @ref DISCORD_AUTO_MODERATION_KEYWORD
   */
    FIELD_STRUCT_PTR(keyword_filter, strings, *)
  /**
   * regular expression patterns which will be matched against content
   * @note associated with @ref DISCORD_AUTO_MODERATION_KEYWORD
   */
    FIELD_STRUCT_PTR(regex_patterns, strings, *)
  /** 
   * the internally pre-defined wordsets which will be searched for in 
   *    content
   * @note associated with @ref DISCORD_AUTO_MODERATION_KEYWORD_PRESET
   */
    FIELD_STRUCT_PTR(presets, integers, *)
  /**
   * substrings which should not trigger the rule
   * @note associated with @ref DISCORD_AUTO_MODERATION_KEYWORD and @ref DISCORD_AUTO_MODERATION_KEYWORD_PRESET
   */
    FIELD_STRUCT_PTR(allow_list, strings, *)
  /**
   * total number of unique role and user mentions allowed per message
   * @note associated with @ref DISCORD_AUTO_MODERATION_MENTION_SPAM
   */
    FIELD(mention_total_limit, int, 0)
  /**
   * whether to automatically detect mention raids
   * @note associated with @ref DISCORD_AUTO_MODERATION_MENTION_SPAM
   */
    FIELD(mention_raid_protection_enabled, bool, false)
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_auto_moderation_action)
  /** the type of action */
    FIELD_ENUM(type, discord_auto_moderation_action_types)
  /** additional metadata needed during execution for this specific action type */
  COND_WRITE(self->metadata != NULL)
    FIELD_STRUCT_PTR(metadata, discord_auto_moderation_action_metadata, *)
  COND_END
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
LIST(discord_auto_moderation_actions)
    LISTTYPE_STRUCT(discord_auto_moderation_action)
LIST_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_auto_moderation_action_metadata)
  /** 
   * channel to which user content should be logged
   * @note associated with @ref DISCORD_AUTO_MODERATION_ACTION_SEND_ALERT_MESSAGE
   */
    FIELD_SNOWFLAKE(channel_id)
  /**
   * timeout duration in seconds
   * @note associated with @ref DISCORD_AUTO_MODERATION_ACTION_TIMEOUT
   * @note maximum of 2419200 seconds (4 weeks)
   */
    FIELD(duration_seconds, int, 0)
  /**
   * additional explanation that will be shown to members 
   *  whenever their message is blocked
   * @note associated with @ref DISCORD_AUTO_MODERATION_ACTION_BLOCK_MESSAGE
   * @note maximum of 150 characters
   */
  COND_WRITE(self->custom_message != NULL)
    FIELD_PTR(custom_message, char, *)
  COND_END
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_auto_moderation_rule} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_auto_moderation_rule)
  /** the ID of this rule */
    FIELD_SNOWFLAKE(id)
  /** the guild which this rule belongs to */
    FIELD_SNOWFLAKE(guild_id)
  /** the rule name */
    FIELD_PTR(name, char, *)
  /** the ID of the user which first created this rule */
    FIELD_SNOWFLAKE(creator_id)
  /** the rule event type */
    FIELD_ENUM(event_type, discord_auto_moderation_event_types)
  /** the rule trigger type */
    FIELD_ENUM(trigger_type, discord_auto_moderation_trigger_types)
  /** the actions which will execute when the rule is triggered */
    FIELD_STRUCT_PTR(actions, discord_auto_moderation_actions, *)
  /** the actions which will execute when the rule is triggered */
    FIELD_STRUCT_PTR(trigger_metadata, discord_auto_moderation_trigger_metadata, *)
  /** whether the rule is enabled */
    FIELD(enabled, bool, false)
  /** the role ids that should not be affected by the rule (Maximum of 20) */
    FIELD_STRUCT_PTR(exempt_roles, snowflakes, *)
  /** the channel ids that should not be affected by the rule (Maximum of 50) */
    FIELD_STRUCT_PTR(exempt_channels, snowflakes, *)
STRUCT_END
#endif

/** @CCORD_pub_list{discord_auto_moderation_rules} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_LIST(discord_auto_moderation_rules)
    LISTTYPE_STRUCT(discord_auto_moderation_rule)
LIST_END
#endif

/*****************************************************************************
 * Auto Moderation REST parameters
 * **************************************************************************/

/** @CCORD_pub_struct{discord_create_auto_moderation_rule} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_create_auto_moderation_rule)
  /** @CCORD_reason{reason} */
#if GENCODECS_RECIPE == DATA
    FIELD_PTR(reason, char, *)
#endif
  /** the rule name */
    FIELD_PTR(name, char, *)
  /** the rule event type */
    FIELD_ENUM(event_type, discord_auto_moderation_event_types)
  /** the rule trigger type */
  COND_WRITE(self->trigger_type != 0)
    FIELD_ENUM(trigger_type, discord_auto_moderation_trigger_types)
  COND_END
  /** the actions which will execute when the rule is triggered */
  COND_WRITE(self->trigger_metadata != NULL)
    FIELD_STRUCT_PTR(trigger_metadata, discord_auto_moderation_trigger_metadata, *)
  COND_END
  /** the actions which will execute when the rule is triggered */
    FIELD_STRUCT_PTR(actions, discord_auto_moderation_actions, *)
  /** whether the rule is enabled */
  COND_WRITE(self->enabled != false)
    FIELD(enabled, bool, false)
  COND_END
  /** the role ids that should not be affected by the rule (Maximum of 20) */
  COND_WRITE(self->exempt_roles != NULL)
    FIELD_STRUCT_PTR(exempt_roles, snowflakes, *)
  COND_END
  /** the channel ids that should not be affected by the rule (Maximum of 50) */
  COND_WRITE(self->exempt_channels != NULL)
    FIELD_STRUCT_PTR(exempt_channels, snowflakes, *)
  COND_END
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_modify_auto_moderation_rule} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_modify_auto_moderation_rule)
  /** @CCORD_reason{reason} */
#if GENCODECS_RECIPE == DATA
    FIELD_PTR(reason, char, *)
#endif
  /** the rule name */
    FIELD_PTR(name, char, *)
  /** the rule event type */
    FIELD_ENUM(event_type, discord_auto_moderation_event_types)
  /** the actions which will execute when the rule is triggered */
  COND_WRITE(self->trigger_metadata != NULL)
    FIELD_STRUCT_PTR(trigger_metadata, discord_auto_moderation_trigger_metadata, *)
  COND_END
  /** the actions which will execute when the rule is triggered */
  COND_WRITE(self->actions != NULL)
    FIELD_STRUCT_PTR(actions, discord_auto_moderation_actions, *)
  COND_END
  /** whether the rule is enabled */
    FIELD(enabled, bool, false)
  /** the role ids that should not be affected by the rule (Maximum of 20) */
  COND_WRITE(self->exempt_roles != NULL)
    FIELD_STRUCT_PTR(exempt_roles, snowflakes, *)
  COND_END
  /** the channel ids that should not be affected by the rule (Maximum of 50) */
  COND_WRITE(self->exempt_channels != NULL)
    FIELD_STRUCT_PTR(exempt_channels, snowflakes, *)
  COND_END
STRUCT_END
#endif

#if GENCODECS_RECIPE == DATA
STRUCT(discord_delete_auto_moderation_rule)
  /** @CCORD_reason{reason} */
    FIELD_PTR(reason, char, *)
STRUCT_END
#endif