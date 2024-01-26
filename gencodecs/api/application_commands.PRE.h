/*****************************************************************************
 * Application Commands Datatypes
 * **************************************************************************/

#if GENCODECS_RECIPE == DATA
ENUM(discord_application_command_types)
  /** Slash commands: a text-based command that shows up when a user 
       types `/` */
    ENUMERATOR(DISCORD_APPLICATION_CHAT_INPUT, = 1)
  /** A UI-based command that shows up when a user
       right clicks or taps on another user */
    ENUMERATOR(DISCORD_APPLICATION_USER, = 2)
  /** A UI-based command that shows up when a user
       right clicks or tap on a message */
    ENUMERATOR_LAST(DISCORD_APPLICATION_MESSAGE, = 3)
ENUM_END
#endif

#if GENCODECS_RECIPE == DATA
ENUM(discord_application_command_option_types)
    ENUMERATOR(DISCORD_APPLICATION_OPTION_SUB_COMMAND, = 1)
    ENUMERATOR(DISCORD_APPLICATION_OPTION_SUB_COMMAND_GROUP, = 2)
    ENUMERATOR(DISCORD_APPLICATION_OPTION_STRING, = 3)
  /** Any integer between `-2^53` and `2^53` */
    ENUMERATOR(DISCORD_APPLICATION_OPTION_INTEGER, = 4)
    ENUMERATOR(DISCORD_APPLICATION_OPTION_BOOLEAN, = 5)
    ENUMERATOR(DISCORD_APPLICATION_OPTION_USER, = 6)
  /** Includes all channel types + categories */
    ENUMERATOR(DISCORD_APPLICATION_OPTION_CHANNEL, = 7)
    ENUMERATOR(DISCORD_APPLICATION_OPTION_ROLE, = 8)
  /** Includes users and roles */
    ENUMERATOR(DISCORD_APPLICATION_OPTION_MENTIONABLE, = 9)
  /** Any double between `-2^53` and `2^53` */
    ENUMERATOR(DISCORD_APPLICATION_OPTION_NUMBER, = 10)
  /** @ref discord_attachment object */
    ENUMERATOR_LAST(DISCORD_APPLICATION_OPTION_ATTACHMENT, = 11)
ENUM_END
#endif

#if GENCODECS_RECIPE == DATA
ENUM(discord_application_command_permission_types)
    ENUMERATOR(DISCORD_APPLICATION_PERMISSION_ROLE, = 1)
    ENUMERATOR(DISCORD_APPLICATION_PERMISSION_USER, = 2)
    ENUMERATOR_LAST(DISCORD_APPLICATION_PERMISSION_CHANNEL, = 3)
ENUM_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_application_command)
  /** unique ID of the command */
  COND_WRITE(self->id != 0)
    FIELD_SNOWFLAKE(id)
  COND_END
  /** one of application command types */
  COND_WRITE(self->type != 0)
    FIELD_ENUM(type, discord_application_command_types)
  COND_END
  /** unique ID of the parent application */
  COND_WRITE(self->application_id != 0)
    FIELD_SNOWFLAKE(application_id)
  COND_END
  /** guild ID of the command, if not global */
  COND_WRITE(self->guild_id != 0)
    FIELD_SNOWFLAKE(guild_id)
  COND_END
  /** name of the command, 1-32 characters */
    FIELD_PTR(name, char, *)
  /** localization dictionary for the `name` field. Values follow the same
       restriction as `name` */
  COND_WRITE(self->name_localizations != NULL)
    FIELD_STRUCT_PTR(name_localizations, strings, *)
  COND_END
  /** 1-100 character description */
    FIELD_PTR(description, char, *)
  /** localization dictionary for the `description` field. Values follow the
       same restriction as `description` */
  COND_WRITE(self->description_localizations != NULL)
    FIELD_STRUCT_PTR(description_localizations, strings, *)
  COND_END
  /** the parameters for the command, max 25 */
  COND_WRITE(self->options != NULL)
    FIELD_STRUCT_PTR(options, discord_application_command_options, *)
  COND_END
  /** Set of @ref DiscordPermissions represented as a bit set */
  COND_WRITE(self->default_member_permissions != 0)
    FIELD_BITMASK(default_member_permissions)
  COND_END
  /** 
   * Indicates whether the command is available in DMs with the app, only
   *    for globally-scoped commands. By default, commands are invisible.
   */
  COND_WRITE(self->dm_permission != false)
    FIELD(dm_permission, bool, false)
  COND_END
  /** @deprecated use `default_member_permissions` instead */
  COND_WRITE(self->default_permission != true)
    FIELD(default_permission, bool, true)
  COND_END
  /** indicates whether the command is age-restricted */
  COND_WRITE(self->nsfw != false)
    FIELD(nsfw, bool, false)
  COND_END
  /** autoincrementing version identifier updated during substantial
       record changes */
  COND_WRITE(self->version != 0)
    FIELD_SNOWFLAKE(version)
  COND_END
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
PUB_LIST(discord_application_commands)
    LISTTYPE_STRUCT(discord_application_command)
LIST_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_application_command_option)
  /** value of application command option type */
    FIELD_ENUM(type, discord_application_command_option_types)
  /** name of the command, 1-32 characters */
    FIELD_PTR(name, char, *)
  /** localization dictionary for the `name` field. Values follow the same
       restriction as `name` */
  COND_WRITE(self->name_localizations != NULL)
    FIELD_STRUCT_PTR(name_localizations, strings, *)
  COND_END
  /** 1-100 character description */
    FIELD_PTR(description, char, *)
  /** localization dictionary for the `description` field. Values follow the
       same restriction as `description` */
  COND_WRITE(self->description_localizations != NULL)
    FIELD_STRUCT_PTR(description_localizations, strings, *)
  COND_END
  /** if the parameter is required or optional -- default `false` */
  COND_WRITE(self->required != false)
    FIELD(required, bool, false)
  COND_END
  /** choices for string and int types for the user to pick from */
  COND_WRITE(self->choices != NULL)
    FIELD_STRUCT_PTR(choices, discord_application_command_option_choices, *)
  COND_END
  /** if the option is a subcommand or subcommand group type, this nested
       options will be the parameters */
  COND_WRITE(self->options != NULL)
    FIELD_STRUCT_PTR(options, discord_application_command_options, *)
  COND_END
  /** if the option is a channel type, the channels shown will be restricted
       to these types */
  COND_WRITE(self->channel_types != NULL)
    FIELD_STRUCT_PTR(channel_types, integers, *)
  COND_END
  /** if the option is an INTEGER or NUMBER type, the minimum value permitted */
  COND_WRITE(self->min_value != 0)
    FIELD(min_value, int, 0)
  COND_END
  /** if the option is an INTEGER or NUMBER type, the maximum value permitted */
  COND_WRITE(self->max_value != 0)
    FIELD(max_value, int, 0)
  COND_END
  /** for option type STRING, the minimum allowed length */
  COND_WRITE(self->min_length != 0)
    FIELD(min_length, int, 0)
  COND_END
  /** for option type STRING, the maximum allowed length */
  COND_WRITE(self->max_length != 0)
    FIELD(max_length, int, 0)
  COND_END
  /** if autocomplete interactions are enabled for this STRING, INTEGER,
       or NUMBER type option */
  COND_WRITE(self->autocomplete != 0)
    FIELD(autocomplete, bool, false)
  COND_END
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
LIST(discord_application_command_options)
    LISTTYPE_STRUCT(discord_application_command_option)
LIST_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_application_command_option_choice)
  /** 1-100 character choice name */
    FIELD_PTR(name, char, *)
  /** localization dictionary for the `name` field. Values follow the same
       restriction as `name` */
  COND_WRITE(self->name_localizations != NULL)
    FIELD_STRUCT_PTR(name_localizations, strings, *)
  COND_END
  /** value of the choice, up to 100 characters if string @note in case of a
       string the value must be enclosed with escaped commas, ex: `\"hi\"` */
    FIELD_PTR(value, json_char, *)
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
LIST(discord_application_command_option_choices)
    LISTTYPE_STRUCT(discord_application_command_option_choice)
LIST_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_application_command_interaction_data_option)
  /** the name of the parameter */
    FIELD_PTR(name, char, *)
  /** value of the application command option type */
    FIELD_ENUM(type, discord_application_command_option_types)
  /** the value of the option resulting from user input @note in case of a
       string the value must be enclosed with escaped commands, ex: `\"hi\"` */
  COND_WRITE(self->value != NULL && *self->value != '\0')
    FIELD_PTR(value, json_char, *)
  COND_END
  /** present if this option is a group or subcommand */
  COND_WRITE(self->options != NULL)
    FIELD_STRUCT_PTR(options, discord_application_command_interaction_data_options, *)
  COND_END
  /** true if this option is the currently focused option for autocomplete */
    FIELD(focused, bool, false)
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
LIST(discord_application_command_interaction_data_options)
    LISTTYPE_STRUCT(discord_application_command_interaction_data_option)
LIST_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_guild_application_command_permission)
  /** the ID of the command */
    FIELD_SNOWFLAKE(id)
  /** the ID of the application the command belongs to */
    FIELD_SNOWFLAKE(application_id)
  /** the ID of the guild */
    FIELD_SNOWFLAKE(guild_id)
  /** the permissions for the command in the guild */
    FIELD_STRUCT_PTR(permissions, discord_application_command_permissions, *)
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
PUB_LIST(discord_guild_application_command_permissions)
    LISTTYPE_STRUCT(discord_guild_application_command_permission)
LIST_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_application_command_permission)
  /** the ID of the role or user */
    FIELD_SNOWFLAKE(id)
  /** role or user */
    FIELD_ENUM(type, discord_application_command_permission_types)
  /** `true` to allow, `false` to disallow */
    FIELD(permission, bool, false)
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
PUB_LIST(discord_application_command_permissions)
    LISTTYPE_STRUCT(discord_application_command_permission)
LIST_END
#endif

/*****************************************************************************
 * Application Commands REST parameters
 * **************************************************************************/

#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_create_global_application_command)
  /** name of the command, 1-32 characters */
    FIELD_PTR(name, char, *)
  /** localization dictionary for the `name` field. Values follow the same
       restriction as `name` */
  COND_WRITE(self->name_localizations != NULL)
    FIELD_STRUCT_PTR(name_localizations, strings, *)
  COND_END
  /** 1-100 character description */
    FIELD_PTR(description, char, *)
  /** localization dictionary for the `description` field. Values follow the
       same restriction as `description` */
  COND_WRITE(self->description_localizations != NULL)
    FIELD_STRUCT_PTR(description_localizations, strings, *)
  COND_END
  /** the parameters for the command */
  COND_WRITE(self->options != NULL)
    FIELD_STRUCT_PTR(options, discord_application_command_options, *)
  COND_END
  /** Set of @ref DiscordPermissions represented as a bit set */
  COND_WRITE(self->default_member_permissions != 0)
    FIELD_BITMASK(default_member_permissions)
  COND_END
  /** 
   * Indicates whether the command is available in DMs with the app, only
   *    for globally-scoped commands. By default, commands are invisible.
   */
    FIELD(dm_permission, bool, false)
  /** @deprecated use `default_member_permissions` instead */
    FIELD(default_permission, bool, true)
  /** the type of command, default `1` if not set */
  COND_WRITE(self->type != 0)
    FIELD_ENUM(type, discord_application_command_types)
  COND_END
  /** indicates whether the command is age-restricted */
  COND_WRITE(self->nsfw != false)
    FIELD(nsfw, bool, false)
  COND_END
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_edit_global_application_command)
  /** name of the command, 1-32 characters */
    FIELD_PTR(name, char, *)
  /** localization dictionary for the `name` field. Values follow the same
       restriction as `name` */
  COND_WRITE(self->name_localizations != NULL)
    FIELD_STRUCT_PTR(name_localizations, strings, *)
  COND_END
  /** 1-100 character description */
    FIELD_PTR(description, char, *)
  /** localization dictionary for the `description` field. Values follow the
       same restriction as `description` */
  COND_WRITE(self->description_localizations != NULL)
    FIELD_STRUCT_PTR(description_localizations, strings, *)
  COND_END
  /** the parameters for the command */
  COND_WRITE(self->options != NULL)
    FIELD_STRUCT_PTR(options, discord_application_command_options, *)
  COND_END
  /** Set of @ref DiscordPermissions represented as a bit set */
  COND_WRITE(self->default_member_permissions != 0)
    FIELD_BITMASK(default_member_permissions)
  COND_END
  /** 
   * Indicates whether the command is available in DMs with the app, only
   *    for globally-scoped commands. By default, commands are invisible.
   */
    FIELD(dm_permission, bool, false)
  /** @deprecated use `default_member_permissions` instead */
    FIELD(default_permission, bool, true)
  /** indicates whether the command is age-restricted */
  COND_WRITE(self->nsfw != false)
    FIELD(nsfw, bool, false)
  COND_END
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_create_guild_application_command)
  /** name of the command, 1-32 characters */
    FIELD_PTR(name, char, *)
  /** localization dictionary for the `name` field. Values follow the same
       restriction as `name` */
  COND_WRITE(self->name_localizations != NULL)
    FIELD_STRUCT_PTR(name_localizations, strings, *)
  COND_END
  /** 1-100 character description */
    FIELD_PTR(description, char, *)
  /** localization dictionary for the `description` field. Values follow the
       same restriction as `description` */
  COND_WRITE(self->description_localizations != NULL)
    FIELD_STRUCT_PTR(description_localizations, strings, *)
  COND_END
  /** the parameters for the command */
  COND_WRITE(self->options != NULL)
    FIELD_STRUCT_PTR(options, discord_application_command_options, *)
  COND_END
  /** Set of @ref DiscordPermissions represented as a bit set */
  COND_WRITE(self->default_member_permissions != 0)
    FIELD_BITMASK(default_member_permissions)
  COND_END
  /** 
   * Indicates whether the command is available in DMs with the app, only
   *    for globally-scoped commands. By default, commands are invisible.
   */
    FIELD(dm_permission, bool, false)
  /** @deprecated use `default_member_permissions` instead */
    FIELD(default_permission, bool, true)
  /** the type of command, default `1` if not set */
  COND_WRITE(self->type != 0)
    FIELD_ENUM(type, discord_application_command_types)
  COND_END
  /** indicates whether the command is age-restricted */
  COND_WRITE(self->nsfw != false)
    FIELD(nsfw, bool, false)
  COND_END
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_edit_guild_application_command)
  /** name of the command, 1-32 characters */
    FIELD_PTR(name, char, *)
  /** localization dictionary for the `name` field. Values follow the same
       restriction as `name` */
  COND_WRITE(self->name_localizations != NULL)
    FIELD_STRUCT_PTR(name_localizations, strings, *)
  COND_END
  /** 1-100 character description */
    FIELD_PTR(description, char, *)
  /** localization dictionary for the `description` field. Values follow the
       same restriction as `description` */
  COND_WRITE(self->description_localizations != NULL)
    FIELD_STRUCT_PTR(description_localizations, strings, *)
  COND_END
  /** the parameters for the command */
  COND_WRITE(self->options != NULL)
    FIELD_STRUCT_PTR(options, discord_application_command_options, *)
  COND_END
  /** Set of @ref DiscordPermissions represented as a bit set */
  COND_WRITE(self->default_member_permissions != 0)
    FIELD_BITMASK(default_member_permissions)
  COND_END
  /** @deprecated use `default_member_permissions` instead */
    FIELD(default_permission, bool, true)
  /** indicates whether the command is age-restricted */
  COND_WRITE(self->nsfw != false)
    FIELD(nsfw, bool, false)
  COND_END
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_bulk_overwrite_guild_application_commands)
  /** ID of the command, if known */
    FIELD_SNOWFLAKE(id)
  /** Name of the command, 1-32 characters */
    FIELD_PTR(name, char, *)
  /** Localization dictionary for the `name` field. Values follow the same
   *    restriction as `name` */
  COND_WRITE(self->name_localizations != NULL)
    FIELD_STRUCT_PTR(name_localizations, strings, *)
  COND_END
  /** 1-100 character description */
    FIELD_PTR(description, char, *)
  /** Localization dictionary for the `description` field. Values follow the
   *    same restriction as `description` */
  COND_WRITE(self->description_localizations != NULL)
    FIELD_STRUCT_PTR(description_localizations, strings, *)
  COND_END
  /** the parameters for the command */
  COND_WRITE(self->options != NULL)
    FIELD_STRUCT_PTR(options, discord_application_command_options, *)
  COND_END
  /** Set of @ref DiscordPermissions represented as a bit set */
  COND_WRITE(self->default_member_permissions != 0)
    FIELD_BITMASK(default_member_permissions)
  COND_END
  /** 
   * Indicates whether the command is available in DMs with the app, only
   *    for globally-scoped commands. By default, commands are invisible.
   */
    FIELD(dm_permission, bool, false)
  /** one of application command types */
  COND_WRITE(self->type != 0)
    FIELD_ENUM(type, discord_application_command_types)
  COND_END
STRUCT_END
#endif
