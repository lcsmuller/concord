/*****************************************************************************
 * Entitlement Datatypes
 * **************************************************************************/

/** Discord polls
 * @see https://discord.com/developers/docs/monetization/entitlements */

#if GENCODECS_RECIPE == DATA
ENUM(discord_entitlement_types)
  /** entitlement was purchased by user */
    ENUMERATOR(DISCORD_ENTITLEMENT_PURCHASE, = 1)
  /** entitlement for Discord Nitro subscription */
    ENUMERATOR(DISCORD_ENTITLEMENT_PREMIUM_SUBSCRIPTION, = 2)
  /** entitlement was gifted by developer */
    ENUMERATOR(DISCORD_ENTITLEMENT_DEVELOPER_GIFT, = 3)
  /** entitlement was purchased by a dev in application test mode */
    ENUMERATOR(DISCORD_ENTITLEMENT_TEST_MODE_PURCHASE, = 4)
  /** entitlement was granted when the SKU was free */
    ENUMERATOR(DISCORD_ENTITLEMENT_FREE_PURCHASE, = 5)
  /** entitlement was gifted by another user */
    ENUMERATOR(DISCORD_ENTITLEMENT_USER_GIFT, = 6)
  /** entitlement was claimed by user for free as a Nitro Subscriber */
    ENUMERATOR(DISCORD_ENTITLEMENT_PREMIUM_PURCHASE, = 7)
  /** entitlement was purchased as an app subscription */
    ENUMERATOR_LAST(DISCORD_ENTITLEMENT_APPLICATION_SUBSCRIPTION, = 8)
ENUM_END
#endif

/** @CCORD_pub_struct{discord_entitlement} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_STRUCT(discord_entitlement)
  /** ID of the entitlement */
  FIELD_SNOWFLAKE(id)
  /** ID of the SKU */
  FIELD_SNOWFLAKE(sku_id)
  /** ID of the parent application */
  FIELD_SNOWFLAKE(application_id)
  /** ID of the user that is granted access
       to the entitlement's sku */
  COND_WRITE(self->user_id != 0)
    FIELD_SNOWFLAKE(user_id)
  COND_END
  /** the type of entitlement */
  FIELD_ENUM(type, discord_entitlement_types)
  /** if the entitlement was deleted */
  FIELD(deleted, bool, false)
  /** start date at which the entitlement is valid */
  COND_WRITE(self->start_date != 0)
    FIELD_TIMESTAMP(start_date)
  COND_END
  /** date at which the entitlement is no longer valid */
  COND_WRITE(self->end_date != 0)
    FIELD_TIMESTAMP(end_date)
  COND_END
  /** ID of the guild that is granted access to the entitlement's sku */
  COND_WRITE(self->guild_id != 0)
    FIELD_SNOWFLAKE(guild_id)
  COND_END
  /** for consumable items, whether or not the entitlement has been consumed */
  FIELD(consumed, bool, false)
STRUCT_END
#endif


/** @CCORD_pub_list{discord_entitlements} */
#if GENCODECS_RECIPE & (DATA | JSON)
PUB_LIST(discord_entitlements)
  LISTTYPE_STRUCT(discord_entitlement)
LIST_END

#endif
