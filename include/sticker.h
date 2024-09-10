/**
 * @file sticker.h
 * @author Cogmasters
 * @brief Sticker public functions and datatypes
 */

#ifndef DISCORD_STICKER_H
#define DISCORD_STICKER_H

/** @defgroup DiscordAPISticker Sticker
 * @ingroup DiscordAPI
 * @brief Sticker's public API supported by Concord
 *  @{ */

/**
 * @brief Get a sticker from a given ID
 *
 * @param client the client created with discord_init()
 * @param sticker_id the sticker to be fetched
 * @CCORD_ret_obj{ret,sticker}
 * @CCORD_return
 */
CCORDcode discord_get_sticker(struct discord *client,
                              u64snowflake sticker_id,
                              struct discord_ret_sticker *ret);

/**
 * @brief Get a list of sticker packs available to Nitro subscribers
 *
 * @param client the client created with discord_init()
 * @CCORD_ret_obj{ret,list_nitro_sticker_packs}
 * @CCORD_return
 */
CCORDcode discord_list_nitro_sticker_packs(
    struct discord *client,
    struct discord_ret_list_nitro_sticker_packs *ret);

/**
 * @brief Get stickers for the given guild
 * @note includes `user` fields if the bot has the `MANAGE_EMOJIS_AND_STICKERS`
 *      permission
 *
 * @param client the client created with discord_init()
 * @param guild_id guild to fetch the stickers from
 * @CCORD_ret_obj{ret,stickers}
 * @CCORD_return
 */
CCORDcode discord_list_guild_stickers(struct discord *client,
                                      u64snowflake guild_id,
                                      struct discord_ret_stickers *ret);

/**
 * @brief Get a sticker for the given guild and sticker ID
 * @note includes the `user` field if the bot has the
 *      `MANAGE_EMOJIS_AND_STICKERS` permission
 *
 * @param client the client created with discord_init()
 * @param guild_id the guild where the sticker belongs to
 * @param sticker_id the sticker to be fetched
 * @CCORD_ret_obj{ret,sticker}
 * @CCORD_return
 */
CCORDcode discord_get_guild_sticker(struct discord *client,
                                    u64snowflake guild_id,
                                    u64snowflake sticker_id,
                                    struct discord_ret_sticker *ret);

/**
 * @brief Modify the given sticker
 * @note requires the `MANAGE_EMOJIS_AND_STICKERS` permission
 *
 * @param client the client created with discord_init()
 * @param guild_id the guild where the sticker belongs to
 * @param sticker_id the sticker to be modified
 * @param params the request parameters 
 * @CCORD_ret_obj{ret,sticker}
 * @CCORD_return
 */
CCORDcode discord_modify_guild_sticker(
    struct discord *client,
    u64snowflake guild_id,
    u64snowflake sticker_id,
    struct discord_modify_guild_sticker *params,
    struct discord_ret_sticker *ret);

/**
 * @brief Delete the given sticker
 * @note requires the `MANAGE_EMOJIS_AND_STICKERS` permission
 *
 * @param client the client created with discord_init()
 * @param guild_id the guild where the sticker belongs to
 * @param sticker_id the sticker to be deleted
 * @CCORD_ret{ret}
 * @CCORD_return
 */
CCORDcode discord_delete_guild_sticker(struct discord *client,
                                       u64snowflake guild_id,
                                       u64snowflake sticker_id,
                                       struct discord_delete_guild_sticker *params,
                                       struct discord_ret *ret);

/** @} DiscordAPISticker */

#endif /* DISCORD_STICKER_H */
