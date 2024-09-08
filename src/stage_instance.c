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
discord_create_stage_instance(struct discord *client,
                              struct discord_create_stage_instance *params,
                              struct discord_ret_stage_instance *ret)
{
    struct discord_attributes attr = { 0 };
    struct ccord_szbuf body;
    char buf[1024];

    CCORD_EXPECT(client, params != NULL, CCORD_BAD_PARAMETER, "");
    CCORD_EXPECT(client, params->channel_id != 0, CCORD_BAD_PARAMETER, "");
    CCORD_EXPECT(client, NOT_EMPTY_STR(params->topic),
                 CCORD_BAD_PARAMETER, "");

    body.size = discord_create_stage_instance_to_json(buf, sizeof(buf), params);
    body.start = buf;

    DISCORD_ATTR_INIT(attr, discord_stage_instance, ret, params->reason);

    return discord_rest_run(&client->rest, &attr, &body, HTTP_POST,
                            "/stage-instances");
}

CCORDcode
discord_get_stage_instance(struct discord *client,
                           u64snowflake channel_id,
                           struct discord_ret_stage_instance *ret)
{
    struct discord_attributes attr = { 0 };

    CCORD_EXPECT(client, channel_id != 0, CCORD_BAD_PARAMETER, "");

    DISCORD_ATTR_INIT(attr, discord_stage_instance, ret, NULL);

    return discord_rest_run(&client->rest, &attr, NULL, HTTP_GET,
                            "/stage-instances/%" PRIu64, channel_id);
}

CCORDcode
discord_modify_stage_instance(struct discord *client,
                              u64snowflake channel_id,
                              struct discord_modify_stage_instance *params,
                              struct discord_ret_stage_instance *ret)
{
    struct discord_attributes attr = { 0 };
    struct ccord_szbuf body;
    char buf[1024];

    CCORD_EXPECT(client, channel_id != 0, CCORD_BAD_PARAMETER, "");

    body.size = discord_modify_stage_instance_to_json(buf, sizeof(buf), params);
    body.start = buf;

    DISCORD_ATTR_INIT(attr, discord_stage_instance, ret, params->reason);

    return discord_rest_run(&client->rest, &attr, &body, HTTP_PATCH,
                            "/stage-instances/%" PRIu64, channel_id);
}

CCORDcode
discord_delete_stage_instance(struct discord *client,
                              u64snowflake channel_id,
                              struct discord_delete_stage_instance *params,
                              struct discord_ret *ret)
{
    struct discord_attributes attr = { 0 };

    CCORD_EXPECT(client, channel_id != 0, CCORD_BAD_PARAMETER, "");

    DISCORD_ATTR_BLANK_INIT(attr, ret, params ? params->reason : NULL);

    return discord_rest_run(&client->rest, &attr, NULL, HTTP_DELETE,
                            "/stage-instances/%" PRIu64, channel_id);
}
