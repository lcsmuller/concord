#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "discord.h"
#include "discord-internal.h"
#include "discord-request.h"

/******************************************************************************
 * REST functions
 ******************************************************************************/

CCORDcode
discord_get_answer_voters(struct discord *client,
                          u64snowflake channel_id,
                          u64snowflake poll_id,
                          u64snowflake answer_id,
                          struct discord_ret_poll_answer_voters *ret)
{
    struct discord_attributes attr = { 0 };

    CCORD_EXPECT(client, channel_id != 0, CCORD_BAD_PARAMETER, "");
    CCORD_EXPECT(client, poll_id != 0, CCORD_BAD_PARAMETER, "");
    CCORD_EXPECT(client, answer_id != 0, CCORD_BAD_PARAMETER, "");

    DISCORD_ATTR_INIT(attr, discord_poll_answer_voters, ret, NULL);

    return discord_rest_run(&client->rest, &attr, NULL, HTTP_GET,
                            "/channnels/%" PRIu64 "/polls/%" PRIu64 "/answers/%" PRIu64, channel_id, poll_id, answer_id); 
}

CCORDcode
discord_end_poll(struct discord *client,
                 u64snowflake channel_id,
                 u64snowflake poll_id,
                 struct discord_ret *ret)
{
    struct discord_attributes attr = { 0 };

    CCORD_EXPECT(client, channel_id != 0, CCORD_BAD_PARAMETER, "");
    CCORD_EXPECT(client, poll_id != 0, CCORD_BAD_PARAMETER, "");

    DISCORD_ATTR_BLANK_INIT(attr, ret, NULL);

    return discord_rest_run(&client->rest, &attr, NULL, HTTP_POST,
                            "/channels/%" PRIu64 "/polls/%" PRIu64 "/expire", channel_id, poll_id);
}
