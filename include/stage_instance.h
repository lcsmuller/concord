/**
 * @file stage_instance.h
 * @author Cogmasters
 * @brief Stage Instance public functions and datatypes
 */

#ifndef DISCORD_STAGE_INSTANCE_H
#define DISCORD_STAGE_INSTANCE_H

/** @defgroup DiscordAPIStageInstance Stage Instance
 * @ingroup DiscordAPI
 * @brief Stage Instance's public API supported by Concord
 *  @{ */

/**
 * @brief Creates a new Stage Instance associated to a Stage channel
 * @note requires the user to be a moderator of the Stage channel
 *
 * @param client the client created with discord_init()
 * @param params the request parameters 
 * @CCORD_ret_obj{ret,stage_instance}
 * @CCORD_return
 */
CCORDcode discord_create_stage_instance(
    struct discord *client,
    struct discord_create_stage_instance *params,
    struct discord_ret_stage_instance *ret);

/**
 * @brief Gets the stage instance associated with the Stage channel, if it
 *      exists
 *
 * @param client the client created with discord_init()
 * @param channel_id the stage channel id
 * @CCORD_ret_obj{ret,stage_instance}
 * @CCORD_return
 */
CCORDcode discord_get_stage_instance(struct discord *client,
                                     u64snowflake channel_id,
                                     struct discord_ret_stage_instance *ret);

/**
 * @brief Updates fields of an existing Stage instance
 * @note requires the user to be a moderator of the Stage channel
 *
 * @param client the client created with discord_init()
 * @param channel_id the stage channel id
 * @param params the request parameters 
 * @CCORD_ret_obj{ret,stage_instance}
 * @CCORD_return
 */
CCORDcode discord_modify_stage_instance(
    struct discord *client,
    u64snowflake channel_id,
    struct discord_modify_stage_instance *params,
    struct discord_ret_stage_instance *ret);

/**
 * @brief Deletes the Stage instance
 * @note requires the user to be a moderator of the Stage channel
 *
 * @param client the client created with discord_init()
 * @param channel_id the stage channel to be deleted
 * @CCORD_ret{ret}
 * @CCORD_return
 */
CCORDcode discord_delete_stage_instance(struct discord *client,
                                        u64snowflake channel_id,
                                        struct discord_delete_stage_instance *params,
                                        struct discord_ret *ret);

/** @} DiscordAPIStageInstance */

#endif /* DISCORD_STAGE_INSTANCE_H */
