/*****************************************************************************
 * Interactions Datatypes
 * **************************************************************************/

/* TODO: add comments */
#if GENCODECS_RECIPE == DATA
ENUM(discord_interaction_types)
    ENUMERATOR(DISCORD_INTERACTION_PING, = 1)
    ENUMERATOR(DISCORD_INTERACTION_APPLICATION_COMMAND, = 2)
    ENUMERATOR(DISCORD_INTERACTION_MESSAGE_COMPONENT, = 3)
    ENUMERATOR(DISCORD_INTERACTION_APPLICATION_COMMAND_AUTOCOMPLETE, = 4)
    ENUMERATOR_LAST(DISCORD_INTERACTION_MODAL_SUBMIT, = 5)
ENUM_END
#endif

#if GENCODECS_RECIPE == DATA
ENUM(discord_interaction_context_types)
  /** interaction can be used within servers */
    ENUMERATOR(DISCORD_INTERACTION_GUILD, = 1)
  /** interaction can be used within DMs with the app's bot user */
    ENUMERATOR(DISCORD_INTERACTION_BOT_DM, = 2)
  /** interaction can be used within Group DMs and DMs other than the app's bot user */
    ENUMERATOR_LAST(DISCORD_INTERACTION_PRIVATE_CHANNEL, = 3)
ENUM_END
#endif

#if GENCODECS_RECIPE == DATA
ENUM(discord_interaction_callback_types)
  /** ACK a @ref DISCORD_INTERACTION_PING */
    ENUMERATOR(DISCORD_INTERACTION_PONG, = 1)
  /** respond to an interaction with a message */
    ENUMERATOR(DISCORD_INTERACTION_CHANNEL_MESSAGE_WITH_SOURCE, = 4)
  /** ACK an interaction and edit a response later, the user sees a loading
       state */
    ENUMERATOR(DISCORD_INTERACTION_DEFERRED_CHANNEL_MESSAGE_WITH_SOURCE, = 5)
  /** for components, ACK an interaction and edit the original message later
       the user does not see a loading state */
    ENUMERATOR(DISCORD_INTERACTION_DEFERRED_UPDATE_MESSAGE, = 6)
  /** for components, edit the message the component was attached to */
    ENUMERATOR(DISCORD_INTERACTION_UPDATE_MESSAGE, = 7)
  /** respond to an autocomplete interaction with suggested choices */
    ENUMERATOR(DISCORD_INTERACTION_APPLICATION_COMMAND_AUTOCOMPLETE_RESULT, = 8)
  /** respond to an interaction with a popup modal */
    ENUMERATOR(DISCORD_INTERACTION_MODAL, = 9)
  /** respond to an interaction with an upgrade button,
       only available for apps with monetization enabled */
  ENUMERATOR_LAST(DISCORD_INTERACTION_PREMIUM_REQUIRED, = 10)
ENUM_END
#endif

/** @CCORD_pub_struct{discord_interaction} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_interaction)
  /** ID of the interaction */
    FIELD_SNOWFLAKE(id)
  /** ID of the application this interaction is for */
    FIELD_SNOWFLAKE(application_id)
  /** the type of the interaction */
    FIELD_ENUM(type, discord_interaction_types)
  /** the command data payload */
    FIELD_STRUCT_PTR(data, discord_interaction_data, *)
  /** the guild it was sent from */
    FIELD_SNOWFLAKE(guild_id)
  /** partial channel object that the interaction was sent from */
    FIELD_STRUCT_PTR(channel, discord_channel, *)
  /** the channel it was sent from */
    FIELD_SNOWFLAKE(channel_id)
  /** guild member data for the invoking user, including permissions */
    FIELD_STRUCT_PTR(member, discord_guild_member, *)
  /** user object for the invoking user, if invoked in a DM */
    FIELD_STRUCT_PTR(user, discord_user, *)
  /** a continuation token for responding to the interaction */
    FIELD_PTR(token, char, *)
  /** read-only property, always `1` */
    FIELD(version, int, 1)
  /** for components, the message they were attached to */
    FIELD_STRUCT_PTR(message, discord_message, *)
  /** bitwise set of permissions the app has in the source location
       of the interaction */
    FIELD_PTR(app_permissions, char, *)
  /** the selected language of the invoking user */
    FIELD_PTR(locale, char, *)
  /** the guild preferred locale, if invoked in a guild */
    FIELD_PTR(guild_locale, char, *)
  /** for monetized apps, any entitlements for the invoking user,
       representing access to premium SKUs */
  COND_WRITE(self->entitlements != NULL)
    FIELD_STRUCT_PTR(entitlements, discord_entitlements, *)
  COND_END
  /* TODO: Add "authorizing_integration_owners" -- What do they mean with dictionary? */
  /** context where the interaction was triggered from */
  FIELD_ENUM(context, discord_interaction_context_types)
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_interaction_data)
  /** Those are always present for application commands: */

  /** the ID of the invoked command */
    FIELD_SNOWFLAKE(id)
  /** the name of the invoked command */
    FIELD_PTR(name, char, *)
  /** the type of the invoked command */
    FIELD_ENUM(type, discord_application_command_types)
  /** converted users + roles + channels + attachments */
  COND_WRITE(self->resolved != NULL)
    FIELD_STRUCT_PTR(resolved, discord_resolved_data, *)
  COND_END
  /** the params + values from the user */
  COND_WRITE(self->options != NULL)
    FIELD_STRUCT_PTR(options, discord_application_command_interaction_data_options, *)
  COND_END
  /** the id of the guild the command is registered to */
    FIELD_SNOWFLAKE(guild_id)
  /** the ID of the user or messaged targetted by a user or message command */
    FIELD_SNOWFLAKE(target_id)

  /** Those are always present for select menu components: */

  /** the custom_id of the component */
  COND_WRITE(self->custom_id != NULL)
    FIELD_PTR(custom_id, char, *)
  COND_END
  /** the type of the component */
    FIELD_ENUM(component_type, discord_component_types)
  /** the values the user selected */
  COND_WRITE(self->values != NULL)
    FIELD_STRUCT_PTR(values, strings, *)
  COND_END

  /** This is always present for model submits: */

  /** the values submitted by the user */
  COND_WRITE(self->components != NULL)
    FIELD_STRUCT_PTR(components, discord_components, *)
  COND_END
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_resolved_data)
  /** the IDs and @ref discord_user datatypes */
    FIELD_PTR(users, json_char, *)
  /** the IDs and partial @ref discord_guild_member datatypes */
    FIELD_PTR(members, json_char, *)
  /** the IDs and @ref discord_role datatypes */
    FIELD_PTR(roles, json_char, *)
  /** the IDs and partial @ref discord_channel datatypes */
    FIELD_PTR(channels, json_char, *)
  /** the IDs and partial @ref discord_message datatypes */
    FIELD_PTR(messages, json_char, *)
  /** the IDs and partial @ref discord_attachment datatypes */
    FIELD_PTR(attachments, json_char, *)
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_message_interaction)
  /** ID of the interaction */
    FIELD_SNOWFLAKE(id)
  /** the type of interaction */
    FIELD_ENUM(type, discord_interaction_types)
  /** the name of the application command */
    FIELD_PTR(name, char, *)
  /** the user who invoked the interaction */
    FIELD_STRUCT_PTR(user, discord_user, *)
  /** the member who invoked the interaction in the guild */
    FIELD_STRUCT_PTR(member, discord_guild_member, *)
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_interaction_response} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_interaction_response)
  /** interaction callback type */
    FIELD_ENUM(type, discord_interaction_callback_types)
  /** an optional response message */
  COND_WRITE(self->data != NULL)
    FIELD_STRUCT_PTR(data, discord_interaction_callback_data, *)
  COND_END
STRUCT_END
#endif

#if GENCODECS_RECIPE & (DATA | JSON)
STRUCT(discord_interaction_callback_data)
  /** message components */
  COND_WRITE(self->components != NULL)
    FIELD_STRUCT_PTR(components, discord_components, *)
  COND_END

  /* Those are only available for Messages: */

  /** is the response TTS */
  COND_WRITE(self->tts != false)
    FIELD(tts, bool, false)
  COND_END
  /** message content */
  COND_WRITE(self->content != NULL)
    FIELD_PTR(content, char, *)
  COND_END
  /** supports up to 10 embeds */
  COND_WRITE(self->embeds != NULL)
    FIELD_STRUCT_PTR(embeds, discord_embeds, *)
  COND_END
  /** @ref DiscordAPIChannelMessageFlags combined as a bitfield (only
       @ref DISCORD_MESSAGE_SUPRESS_EMBEDS and @ref DISCORD_MESSAGE_EPHEMERAL
       can be set) */
  COND_WRITE(self->flags != 0)
    FIELD_BITMASK(flags)
  COND_END
  /** attachment objects with filename and description */
  COND_WRITE(self->attachments != NULL)
    FIELD_STRUCT_PTR(attachments, discord_attachments, *)
  COND_END
  /* AUTOCOMPLETE */
  /** autocomplete choices (max of 25 choices) */
    FIELD_STRUCT_PTR(choices, discord_application_command_option_choices, *)
  /* MODAL */
  /** a developer defined identifier for the component, max 100 characters */
    FIELD_PTR(custom_id, char, *)
  /** the title of the popup modal */
    FIELD_PTR(title, char, *)
STRUCT_END
#endif

/*****************************************************************************
 * Interactions REST parameters
 * **************************************************************************/

/** @CCORD_pub_struct{discord_edit_original_interaction_response} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_edit_original_interaction_response)
  /* QUERY FIELDS */
#if !(GENCODECS_RECIPE & JSON)
  /** id of the thread the message is in */
    FIELD_SNOWFLAKE(thread_id)
#endif
  /* JSON FIELDS */
  /** the message contents (up to 2000 characters) */
    FIELD_PTR(content, char, *)
  /** embedded `rich` content */
  COND_WRITE(self->embeds != NULL)
    FIELD_STRUCT_PTR(embeds, discord_embeds, *)
  COND_END
  /** allowed mentions for the message */
  COND_WRITE(self->allowed_mentions != NULL)
    FIELD_STRUCT_PTR(allowed_mentions, discord_allowed_mention, *)
  COND_END
  /** the components to include with the message */
  COND_WRITE(self->components != NULL)
    FIELD_STRUCT_PTR(components, discord_components, *)
  COND_END
  /** attached files to keep and possible descriptions for new files */
  COND_WRITE(self->attachments != NULL)
    FIELD_STRUCT_PTR(attachments, discord_attachments, *)
  COND_END
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_create_followup_message} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_create_followup_message)
  /* QUERY FIELDS */
#if !(GENCODECS_RECIPE & JSON)
  /** waits for server confirmation of message send before response, and
       returns the created message body (defaults to `false`; when `false` a
       message that is not saved does not return an error) */
    FIELD(wait, bool, true)
  /** send a message to the specified thread within a webhook's channel; the
       thread will automatically be unarchived */
    FIELD_SNOWFLAKE(thread_id)
#endif
  /* JSON FIELDS */
  /** override the default avatar of the webhook */
    FIELD_PTR(avatar_url, char, *)
  /** true if this is a TTS message */
    FIELD(tts, bool, false)
  /** embedded `rich` content */
  COND_WRITE(self->embeds != NULL)
    FIELD_STRUCT_PTR(embeds, discord_embeds, *)
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
  /** @ref DiscordAPIChannelMessageFlags combined as a bitfield (only 
       `SUPPRESS_EMBEDS` can be set) */
  COND_WRITE(self->flags != 0)
    FIELD_BITMASK(flags)
  COND_END
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_edit_followup_message} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_edit_followup_message)
  /* QUERY FIELDS */
#if !(GENCODECS_RECIPE & JSON)
  /** id of the thread the message is in */
    FIELD_SNOWFLAKE(thread_id)
#endif
  /* JSON FIELDS */
  /** the message contents (up to 2000 characters) */
    FIELD_PTR(content, char, *)
  /** embedded `rich` content */
  COND_WRITE(self->embeds != NULL)
    FIELD_STRUCT_PTR(embeds, discord_embeds, *)
  COND_END
  /** allowed mentions for the message */
  COND_WRITE(self->allowed_mentions != NULL)
    FIELD_STRUCT_PTR(allowed_mentions, discord_allowed_mention, *)
  COND_END
  /** the components to include with the message */
  COND_WRITE(self->components != NULL)
    FIELD_STRUCT_PTR(components, discord_components, *)
  COND_END
  /** attached files to keep and possible descriptions for new files */
  COND_WRITE(self->attachments != NULL)
    FIELD_STRUCT_PTR(attachments, discord_attachments, *)
  COND_END
STRUCT_END
#endif
