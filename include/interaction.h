/**
 * @file interaction.h
 * @author Cogmasters
 * @brief Interaciton public functions and datatypes
 */

#ifndef DISCORD_INTERACTION_H
#define DISCORD_INTERACTION_H

/** @defgroup DiscordAPIInteractionsReact Receiving and sending
 * @ingroup DiscordAPIInteractions
 * @brief Receiving and sending interactions
 *  @{ */

/**
 * @brief Create a response to an Interaction from the gateway
 *
 * @param client the client created with discord_init()
 * @param interaction_id the unique id of the interaction
 * @param interaction_token the unique token of the interaction
 * @param params the request parameters
 * @CCORD_ret_obj{ret,interaction_response}
 * @CCORD_return
 */
CCORDcode discord_create_interaction_response(
    struct discord *client,
    u64snowflake interaction_id,
    const char interaction_token[],
    struct discord_interaction_response *params,
    struct discord_ret_interaction_response *ret);

/**
 * @brief Get the initial Interaction response
 *
 * @param client the client created with discord_init()
 * @param application_id the unique id of the application
 * @param interaction_token the unique token of the interaction
 * @CCORD_ret_obj{ret,interaction}
 * @CCORD_return
 */
CCORDcode discord_get_original_interaction_response(
    struct discord *client,
    u64snowflake application_id,
    const char interaction_token[],
    struct discord_ret_message *ret);

/**
 * @brief Edit the initial Interaction response
 *
 * @param client the client created with discord_init()
 * @param application_id the unique id of the application
 * @param interaction_token the unique token of the interaction
 * @param params request parameters
 * @CCORD_ret_obj{ret,interaction_response}
 * @CCORD_return
 */
CCORDcode discord_edit_original_interaction_response(
    struct discord *client,
    u64snowflake application_id,
    const char interaction_token[],
    struct discord_edit_original_interaction_response *params,
    struct discord_ret_interaction_response *ret);

/**
 * @brief Delete the initial Interaction response
 *
 * @param client the client created with discord_init()
 * @param application_id the unique id of the application
 * @param interaction_token the unique token of the interaction
 * @CCORD_ret{ret}
 * @CCORD_return
 */
CCORDcode discord_delete_original_interaction_response(
    struct discord *client,
    u64snowflake application_id,
    const char interaction_token[],
    struct discord_ret *ret);

/**
 * @brief Create a followup message for an Interaction
 *
 * @param client the client created with discord_init()
 * @param application_id the unique id of the application
 * @param interaction_token the unique token of the interaction
 * @param params request parameters
 * @CCORD_ret_obj{ret,webhook}
 * @CCORD_return
 */
CCORDcode discord_create_followup_message(
    struct discord *client,
    u64snowflake application_id,
    const char interaction_token[],
    struct discord_create_followup_message *params,
    struct discord_ret_webhook *ret);

/**
 * @brief Get a followup message for an interaction
 *
 * @param client the client created with discord_init()
 * @param application_id the unique id of the application
 * @param interaction_token the unique token of the interaction
 * @param message_id the unique id of the message
 * @CCORD_ret_obj{ret,message}
 * @CCORD_return
 */
CCORDcode discord_get_followup_message(struct discord *client,
                                       u64snowflake application_id,
                                       const char interaction_token[],
                                       u64snowflake message_id,
                                       struct discord_ret_message *ret);

/**
 * @brief Edits a followup message for an interaction
 *
 * @param client the client created with discord_init()
 * @param application_id the unique id of the application
 * @param interaction_token the unique token of the interaction
 * @param message_id the unique id of the message
 * @param params request parameters
 * @CCORD_ret_obj{ret,message}
 * @CCORD_return
 */
CCORDcode discord_edit_followup_message(
    struct discord *client,
    u64snowflake application_id,
    const char interaction_token[],
    u64snowflake message_id,
    struct discord_edit_followup_message *params,
    struct discord_ret_message *ret);

/**
 * @brief Edits a followup message for an interaction
 *
 * @param client the client created with discord_init()
 * @param application_id the unique id of the application
 * @param interaction_token the unique token of the interaction
 * @param message_id the unique id of the message
 * @CCORD_ret{ret}
 * @CCORD_return
 */
CCORDcode discord_delete_followup_message(struct discord *client,
                                          u64snowflake application_id,
                                          const char interaction_token[],
                                          u64snowflake message_id,
                                          struct discord_ret *ret);

/** @example components.c
 * Demonstrates a couple use cases of the Message Components API */

/** @} DiscordAPIInteractionsReact */

#endif /* DISCORD_INTERACTION_H */
