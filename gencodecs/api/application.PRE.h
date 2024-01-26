/*****************************************************************************
 * Application Datatypes
 * **************************************************************************/

/** @defgroup DiscordApplicationFlags Application flags
 * @ingroup DiscordConstants
 *  @{ */

/** indicates if an app uses the Auto Moderation API */
PP_DEFINE(DISCORD_APPLICATION_APPLICATION_AUTO_MODERATION_RULE_CREATE_BADGE 1 << 6)
/**	intent required for bots in 100 or more servers to
      receive presence_update events */
PP_DEFINE(DISCORD_APPLICATION_GATEWAY_PRESENCE 1 << 12)
/** intent required for bots in under 100 servers to receive
      presence_update events */
PP_DEFINE(DISCORD_APPLICATION_GATEWAY_PRESENCE_LIMITED 1 << 13)
/** intent required for bots in 100 or more servers to receive
      member-related events like guild_member_add */
PP_DEFINE(DISCORD_APPLICATION_GATEWAY_GUILD_MEMBERS 1 << 14)
/** intent required for bots in under 100 servers to receive
      member-related events like guild_member_add */
PP_DEFINE(DISCORD_APPLICATION_GATEWAY_GUILD_MEMBERS_LIMITED 1 << 15)
/** indicates unusual growth of an app that prevents verification */
PP_DEFINE(DISCORD_APPLICATION_VERIFICATION_PENDING_GUILD_LIMIT 1 << 16)
/** indicates if an app is embedded within the Discord client */
PP_DEFINE(DISCORD_APPLICATION_EMBEDDED 1 << 17)
/** intent required for bots in 100 or more servers to receive message content */
PP_DEFINE(DISCORD_APPLICATION_GATEWAY_MESSAGE_CONTENT 1 << 18)
/** intent required for bots in under 100 servers to receive message content */
PP_DEFINE(DISCORD_APPLICATION_GATEWAY_MESSAGE_CONTENT_LIMITED 1 << 19)
/** indicates if an app has registered global application commands */
PP_DEFINE(DISCORD_APPLICATION_APPLICATION_COMMAND_BADGE 1 << 23)

/** @} DiscordApplicationFlags */

/** @CCORD_pub_struct{discord_application} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_application)
  /** the ID of the app */
    FIELD_SNOWFLAKE(id)
  /** the name of the app */
    FIELD_PTR(name, char, *)
  /** the icon hash of the app */
    FIELD_PTR(icon, char, *)
  /** the description of the app */
    FIELD_PTR(description, char, *)
  /** an array of rpc origin urls, if rpc is enabled */
  COND_WRITE(self->rpc_origins != NULL)
    FIELD_STRUCT_PTR(rpc_origins, strings, *)
  COND_END
  /** when false only app owner can join the app's bot to guilds */
    FIELD(bot_public, bool, false)
  /** when true the app's bot will only join upon completion of the full
       oauth2 code grant flow */
    FIELD(bot_require_code_grant, bool, false)
  /** the url of the app's terms of service */
    FIELD_PTR(terms_of_service_url, char, *)
  /** the url of the app's privacy policy */
    FIELD_PTR(privacy_policy_url, char, *)
  /** partial user object containing info on the owner of the application */
  COND_WRITE(self->owner != NULL)
    FIELD_STRUCT_PTR(owner, discord_user, *)
  COND_END
  /** @deprecated will be removed in API v11 */
    FIELD_PTR(summary, char, *)
  /** the hex encoded key for verification in interactions and the GameSDK's
       GetTicket */
    FIELD_PTR(verify_key, char, *)
  /** if the application belongs to a team, this will be a list of the
       members of that team */
  COND_WRITE(self->team != NULL)
    FIELD_STRUCT_PTR(team, discord_team, *)
  COND_END
  /** if this application is a game sold on Discord, this field will be the
       guild to which it has been linked */
    FIELD_SNOWFLAKE(guild_id)
  /** Partial object of the associated guild */
  COND_WRITE(self->guild != NULL)
    FIELD_STRUCT_PTR(guild, discord_guild, *)
  COND_END
  /** if this application is a game sold on Discord, this field will be the
       ID of the \"Game SKU\" that is created, if exists */
    FIELD_SNOWFLAKE(primary_sku_id)
  /** if this application is a game sold on Discord, this field will be the
       URL slug that links to the store page */
    FIELD_PTR(slug, char, *)
  /** the application's default rich presence invite cover image hash */
    FIELD_PTR(cover_image, char, *)
  /** the application's public flags @see DiscordApplicationFlags */
    FIELD_BITMASK(flags)
  /** Approximate count of guilds the app has been added to */
    FIELD(guild_count, size_t, 0)
  /** Array of redirect URIs for the app */
  COND_WRITE(self->redirect_uris != NULL)
    FIELD_STRUCT_PTR(redirect_uris, strings, *)
  COND_END
STRUCT_END
#endif

/** @CCORD_pub_struct{discord_install_params} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_install_params)
  /** the scopes to add the application to the server with */
    FIELD_STRUCT_PTR(scopes, strings, *)
  /** the permissions to request for the bot role */
    FIELD_BITMASK(permissions)
STRUCT_END
#endif
