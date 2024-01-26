/*****************************************************************************
 * Audit Logs Datatypes
 * **************************************************************************/

#if GENCODECS_RECIPE == DATA
ENUM(discord_audit_log_events)
    ENUMERATOR(DISCORD_AUDIT_LOG_GUILD_UPDATE, = 1)
    ENUMERATOR(DISCORD_AUDIT_LOG_CHANNEL_CREATE, = 10)
    ENUMERATOR(DISCORD_AUDIT_LOG_CHANNEL_UPDATE, = 11)
    ENUMERATOR(DISCORD_AUDIT_LOG_CHANNEL_DELETE, = 12)
    ENUMERATOR(DISCORD_AUDIT_LOG_CHANNEL_OVERWRITE_CREATE, = 13)
    ENUMERATOR(DISCORD_AUDIT_LOG_CHANNEL_OVERWRITE_UPDATE, = 14)
    ENUMERATOR(DISCORD_AUDIT_LOG_CHANNEL_OVERWRITE_DELETE, = 15)
    ENUMERATOR(DISCORD_AUDIT_LOG_MEMBER_KICK, = 20)
    ENUMERATOR(DISCORD_AUDIT_LOG_MEMBER_PRUNE, = 21)
    ENUMERATOR(DISCORD_AUDIT_LOG_MEMBER_BAN_ADD, = 22)
    ENUMERATOR(DISCORD_AUDIT_LOG_MEMBER_BAN_REMOVE, = 23)
    ENUMERATOR(DISCORD_AUDIT_LOG_MEMBER_UPDATE, = 24)
    ENUMERATOR(DISCORD_AUDIT_LOG_MEMBER_ROLE_UPDATE, = 25)
    ENUMERATOR(DISCORD_AUDIT_LOG_MEMBER_MOVE, = 26)
    ENUMERATOR(DISCORD_AUDIT_LOG_MEMBER_DISCONNECT, = 27)
    ENUMERATOR(DISCORD_AUDIT_LOG_BOT_ADD, = 28)
    ENUMERATOR(DISCORD_AUDIT_LOG_ROLE_CREATE, = 30)
    ENUMERATOR(DISCORD_AUDIT_LOG_ROLE_UPDATE, = 31)
    ENUMERATOR(DISCORD_AUDIT_LOG_ROLE_DELETE, = 32)
    ENUMERATOR(DISCORD_AUDIT_LOG_INVITE_CREATE, = 40)
    ENUMERATOR(DISCORD_AUDIT_LOG_INVITE_UPDATE, = 41)
    ENUMERATOR(DISCORD_AUDIT_LOG_INVITE_DELETE, = 42)
    ENUMERATOR(DISCORD_AUDIT_LOG_WEBHOOK_CREATE, = 50)
    ENUMERATOR(DISCORD_AUDIT_LOG_WEBHOOK_UPDATE, = 51)
    ENUMERATOR(DISCORD_AUDIT_LOG_WEBHOOK_DELETE, = 52)
    ENUMERATOR(DISCORD_AUDIT_LOG_EMOJI_CREATE, = 60)
    ENUMERATOR(DISCORD_AUDIT_LOG_EMOJI_UPDATE, = 61)
    ENUMERATOR(DISCORD_AUDIT_LOG_EMOJI_DELETE, = 62)
    ENUMERATOR(DISCORD_AUDIT_LOG_MESSAGE_DELETE, = 72)
    ENUMERATOR(DISCORD_AUDIT_LOG_MESSAGE_BULK_DELETE, = 73)
    ENUMERATOR(DISCORD_AUDIT_LOG_MESSAGE_PIN, = 74)
    ENUMERATOR(DISCORD_AUDIT_LOG_MESSAGE_UNPIN, = 75)
    ENUMERATOR(DISCORD_AUDIT_LOG_INTEGRATION_CREATE, = 80)
    ENUMERATOR(DISCORD_AUDIT_LOG_INTEGRATION_UPDATE, = 81)
    ENUMERATOR(DISCORD_AUDIT_LOG_INTEGRATION_DELETE, = 82)
    ENUMERATOR(DISCORD_AUDIT_LOG_STAGE_INSTANCE_CREATE, = 83)
    ENUMERATOR(DISCORD_AUDIT_LOG_STAGE_INSTANCE_UPDATE, = 84)
    ENUMERATOR(DISCORD_AUDIT_LOG_STAGE_INSTANCE_DELETE, = 85)
    ENUMERATOR(DISCORD_AUDIT_LOG_STICKER_CREATE, = 90)
    ENUMERATOR(DISCORD_AUDIT_LOG_STICKER_UPDATE, = 91)
    ENUMERATOR(DISCORD_AUDIT_LOG_STICKER_DELETE, = 92)
    ENUMERATOR(DISCORD_AUDIT_LOG_GUILD_SCHEDULED_EVENT_CREATE, = 100)
    ENUMERATOR(DISCORD_AUDIT_LOG_GUILD_SCHEDULED_EVENT_UPDATE, = 101)
    ENUMERATOR(DISCORD_AUDIT_LOG_GUILD_SCHEDULED_EVENT_DELETE, = 102)
    ENUMERATOR(DISCORD_AUDIT_LOG_THREAD_CREATE, = 110)
    ENUMERATOR(DISCORD_AUDIT_LOG_THREAD_UPDATE, = 111)
    ENUMERATOR(DISCORD_AUDIT_LOG_THREAD_DELETE, = 112)
    ENUMERATOR(DISCORD_AUDIT_LOG_APPLICATION_COMMAND_PERMISSION_UPDATE, = 121)
    ENUMERATOR(DISCORD_AUDIT_LOG_AUTO_MODERATION_RULE_CREATE, = 140)
    ENUMERATOR(DISCORD_AUDIT_LOG_AUTO_MODERATION_RULE_UPDATE, = 141)
    ENUMERATOR(DISCORD_AUDIT_LOG_AUTO_MODERATION_RULE_DELETE, = 142)
    ENUMERATOR(DISCORD_AUDIT_LOG_AUTO_MODERATION_BLOCK_MESSAGE, = 143)
    ENUMERATOR(DISCORD_AUDIT_LOG_AUTO_MODERATION_FLAG_TO_CHANNEL, = 144)
    ENUMERATOR(DISCORD_AUDIT_LOG_AUTO_MODERATION_USER_COMMUNICATION_DISABLED, = 145)
    ENUMERATOR(DISCORD_AUDIT_LOG_CREATOR_MONETIZATION_REQUEST_CREATED, = 150)
    ENUMERATOR_LAST(DISCORD_AUDIT_LOG_CREATOR_MONETIZATION_TERMS_ACCEPTED, = 151)
ENUM_END
#endif

/** @CCORD_pub_struct{discord_audit_log} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_audit_log)
  /** list of application commands referenced in the audit log */
  COND_WRITE(self->application_commands != NULL)
    FIELD_STRUCT_PTR(application_commands, discord_application_commands, *)
  COND_END
  /** list of audit log entries referenced in the audit log */
  COND_WRITE(self->audit_log_entries != NULL)
    FIELD_STRUCT_PTR(audit_log_entries, discord_audit_log_entries, *)
  COND_END
  /** list of auto moderation rules referenced in the audit log */
  COND_WRITE(self->auto_moderation_rules != NULL)
    FIELD_STRUCT_PTR(auto_moderation_rules, discord_auto_moderation_rules, *)
  COND_END
  /** list of guild scheduled events referenced in the audit log */
  COND_WRITE(self->guild_scheduled_events != NULL)
    FIELD_STRUCT_PTR(guild_scheduled_events, discord_guild_scheduled_events, *)
  COND_END
  /** list of partial integration objects */
  COND_WRITE(self->integrations != NULL)
    FIELD_STRUCT_PTR(integrations, discord_integrations, *)
  COND_END
  /** list of threads found in the audit log */
  COND_WRITE(self->threads != NULL)
    FIELD_STRUCT_PTR(threads, discord_channels, *)
  COND_END
  /** list of users found in the audit log */
  COND_WRITE(self->users != NULL)
    FIELD_STRUCT_PTR(users, discord_users, *)
  COND_END
  /** list of webhooks found in the audit log */
  COND_WRITE(self->webhooks != NULL)
    FIELD_STRUCT_PTR(webhooks, discord_webhooks, *)
  COND_END
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_audit_log_entry} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_audit_log_entry)
  /** ID of the affected entity (webhook, user, role, etc.) */
    FIELD_SNOWFLAKE(target_id)
  /** changes made to the target_id */
  COND_WRITE(self->changes != NULL)
    FIELD_STRUCT_PTR(changes, discord_audit_log_changes, *)
  COND_END
  /** the user who made the changes */
    FIELD_SNOWFLAKE(user_id)
  /** id of the entry */
    FIELD_SNOWFLAKE(id)
  /** type of action that occurred */
  COND_WRITE(self->action_type != 0)
    FIELD_ENUM(action_type, discord_audit_log_events)
  COND_END
  /** additional info for certain action types */
  COND_WRITE(self->options != NULL)
    FIELD_STRUCT_PTR(options, discord_optional_audit_entry_infos, *)
  COND_END
  /** the reason for the change (0-512) characters */
    FIELD_PTR(reason, char, *)
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
LIST(discord_audit_log_entries)
    LISTTYPE_STRUCT(discord_audit_log_entry)
LIST_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_optional_audit_entry_info)
  /** ID of the app whose permissions were targeted */
    FIELD_SNOWFLAKE(application_id)
  /** Name of the Auto Moderation rule that was triggered */
    FIELD_PTR(auto_moderation_rule_name, char, *)
  /** Trigger type of the Auto Moderation rule that was triggered */
    FIELD_PTR(auto_moderation_rule_trigger_type, char, *)
  /** channel in which the entities were targeted */
    FIELD_SNOWFLAKE(channel_id)
  /** number of entities that were targeted */
    FIELD_PTR(count, char, *)
  /** number of days after which inactive members were kicked */
    FIELD_PTR(delete_member_days, char, *)
  /** id of the overwritten entity */
    FIELD_SNOWFLAKE(id)
  /** number of members removed by prune */
    FIELD_PTR(members_removed, char, *)
  /** ID of the message that was targeted */
    FIELD_SNOWFLAKE(message_id)
  /** name of the role if type is \"0\" (not present if type is \"1\" */
    FIELD_PTR(role_name, char, *)
  /** type of overwritten entity - 0 for role or 1 for \"member\" */
    FIELD_PTR(type, char, *)
  /** The type of integration which performed the action */
    FIELD_PTR(integration_type, char, *)
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
LIST(discord_optional_audit_entry_infos)
    LISTTYPE_STRUCT(discord_optional_audit_entry_info)
LIST_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_audit_log_change)
  /** new value of the key */
    FIELD_PTR(new_value, json_char, *)
  /** old value of the key */
    FIELD_PTR(old_value, json_char, *)
  /** name of audit log change key */
    FIELD_PTR(key, char, *)
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
LIST(discord_audit_log_changes)
    LISTTYPE_STRUCT(discord_audit_log_change)
LIST_END
#endif

/*****************************************************************************
 * Audit Logs REST parameters
 * **************************************************************************/

#if GENCODECS_RECIPE == DATA
STRUCT(discord_get_guild_audit_log)
  /** filter the log for actions made by a user */
  COND_WRITE(self->user_id != 0)
    FIELD_SNOWFLAKE(user_id)
  COND_END
  /** the type of audit log event */
  COND_WRITE(self->action_type != 0)
    FIELD(action_type, int, 0)
  COND_END
  /** filter the log before a certain entry ID */
  COND_WRITE(self->before != 0)
    FIELD_SNOWFLAKE(before)
  COND_END
  /** how many entries are returned (default 50, minimum 1, maximum 100) */
  COND_WRITE(self->limit >= 1 && self->limit <= 100)
    FIELD(limit, int, 50)
  COND_END
STRUCT_END
#endif
