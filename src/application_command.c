#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "discord.h"
#include "discord-internal.h"
#include "discord-request.h"

CCORDcode
discord_get_global_application_commands(
    struct discord *client,
    u64snowflake application_id,
    struct discord_ret_application_commands *ret)
{
    struct discord_attributes attr = { 0 };

    CCORD_EXPECT(client, application_id != 0, CCORD_BAD_PARAMETER, "");

    DISCORD_ATTR_LIST_INIT(attr, discord_application_commands, ret, NULL);

    return discord_rest_run(&client->rest, &attr, NULL, HTTP_GET,
                            "/applications/%" PRIu64 "/commands",
                            application_id);
}

CCORDcode
discord_create_global_application_command(
    struct discord *client,
    u64snowflake application_id,
    struct discord_create_global_application_command *params,
    struct discord_ret_application_command *ret)
{
    struct discord_attributes attr = { 0 };
    struct ccord_szbuf body;
    char buf[4096];

    CCORD_EXPECT(client, application_id != 0, CCORD_BAD_PARAMETER, "");
    CCORD_EXPECT(client, params != NULL, CCORD_BAD_PARAMETER, "");
    CCORD_EXPECT(client, NOT_EMPTY_STR(params->name), CCORD_BAD_PARAMETER, "");
    if (params->type != 2 && params->type != 3) {
      CCORD_EXPECT(client, NOT_EMPTY_STR(params->description),
                   CCORD_BAD_PARAMETER, "");
    }

    DISCORD_ATTR_INIT(attr, discord_application_command, ret, NULL);

    body.size = discord_create_global_application_command_to_json(
        buf, sizeof(buf), params);
    body.start = buf;

    return discord_rest_run(&client->rest, &attr, &body, HTTP_POST,
                            "/applications/%" PRIu64 "/commands",
                            application_id);
}

CCORDcode
discord_get_global_application_command(
    struct discord *client,
    u64snowflake application_id,
    u64snowflake command_id,
    struct discord_ret_application_command *ret)
{
    struct discord_attributes attr = { 0 };

    CCORD_EXPECT(client, application_id != 0, CCORD_BAD_PARAMETER, "");
    CCORD_EXPECT(client, command_id != 0, CCORD_BAD_PARAMETER, "");

    DISCORD_ATTR_INIT(attr, discord_application_command, ret, NULL);

    return discord_rest_run(&client->rest, &attr, NULL, HTTP_GET,
                            "/applications/%" PRIu64 "/commands/%" PRIu64,
                            application_id, command_id);
}

CCORDcode
discord_edit_global_application_command(
    struct discord *client,
    u64snowflake application_id,
    u64snowflake command_id,
    struct discord_edit_global_application_command *params,
    struct discord_ret_application_command *ret)
{
    struct discord_attributes attr = { 0 };
    struct ccord_szbuf body;
    char buf[4096];

    CCORD_EXPECT(client, application_id != 0, CCORD_BAD_PARAMETER, "");
    CCORD_EXPECT(client, command_id != 0, CCORD_BAD_PARAMETER, "");

    body.size = discord_edit_global_application_command_to_json(
        buf, sizeof(buf), params);
    body.start = buf;

    DISCORD_ATTR_INIT(attr, discord_application_command, ret, NULL);

    return discord_rest_run(&client->rest, &attr, &body, HTTP_PATCH,
                            "/applications/%" PRIu64 "/commands/%" PRIu64,
                            application_id, command_id);
}

CCORDcode
discord_delete_global_application_command(struct discord *client,
                                          u64snowflake application_id,
                                          u64snowflake command_id,
                                          struct discord_ret *ret)
{
    struct discord_attributes attr = { 0 };

    CCORD_EXPECT(client, application_id != 0, CCORD_BAD_PARAMETER, "");
    CCORD_EXPECT(client, command_id != 0, CCORD_BAD_PARAMETER, "");

    DISCORD_ATTR_BLANK_INIT(attr, ret, NULL);

    return discord_rest_run(&client->rest, &attr, NULL, HTTP_DELETE,
                            "/applications/%" PRIu64 "/commands/%" PRIu64,
                            application_id, command_id);
}

CCORDcode
discord_bulk_overwrite_global_application_commands(
    struct discord *client,
    u64snowflake application_id,
    struct discord_application_commands *params,
    struct discord_ret_application_commands *ret)
{
    struct discord_attributes attr = { 0 };
    struct ccord_szbuf body;
    char buf[8192];

    CCORD_EXPECT(client, application_id != 0, CCORD_BAD_PARAMETER, "");
    CCORD_EXPECT(client, params != NULL, CCORD_BAD_PARAMETER, "");

    body.size = discord_application_commands_to_json(buf, sizeof(buf), params);
    body.start = buf;

    DISCORD_ATTR_LIST_INIT(attr, discord_application_commands, ret, NULL);

    return discord_rest_run(&client->rest, &attr, &body, HTTP_PUT,
                            "/applications/%" PRIu64 "/commands",
                            application_id);
}

CCORDcode
discord_get_guild_application_commands(
    struct discord *client,
    u64snowflake application_id,
    u64snowflake guild_id,
    struct discord_ret_application_commands *ret)
{
    struct discord_attributes attr = { 0 };

    CCORD_EXPECT(client, application_id != 0, CCORD_BAD_PARAMETER, "");
    CCORD_EXPECT(client, guild_id != 0, CCORD_BAD_PARAMETER, "");

    DISCORD_ATTR_LIST_INIT(attr, discord_application_commands, ret, NULL);

    return discord_rest_run(&client->rest, &attr, NULL, HTTP_GET,
                            "/applications/%" PRIu64 "/guilds/%" PRIu64
                            "/commands",
                            application_id, guild_id);
}

CCORDcode
discord_create_guild_application_command(
    struct discord *client,
    u64snowflake application_id,
    u64snowflake guild_id,
    struct discord_create_guild_application_command *params,
    struct discord_ret_application_command *ret)
{
    struct discord_attributes attr = { 0 };
    struct ccord_szbuf body;
    char buf[4096];

    CCORD_EXPECT(client, application_id != 0, CCORD_BAD_PARAMETER, "");
    CCORD_EXPECT(client, guild_id != 0, CCORD_BAD_PARAMETER, "");
    CCORD_EXPECT(client, params != NULL, CCORD_BAD_PARAMETER, "");
    CCORD_EXPECT(client, NOT_EMPTY_STR(params->name), CCORD_BAD_PARAMETER, "");
    if (params->type != 2 && params->type != 3) {
      CCORD_EXPECT(client, NOT_EMPTY_STR(params->description),
                   CCORD_BAD_PARAMETER, "");
    }

    body.size = discord_create_guild_application_command_to_json(
        buf, sizeof(buf), params);
    body.start = buf;

    DISCORD_ATTR_INIT(attr, discord_application_command, ret, NULL);

    return discord_rest_run(&client->rest, &attr, &body, HTTP_POST,
                            "/applications/%" PRIu64 "/guilds/%" PRIu64
                            "/commands",
                            application_id, guild_id);
}

CCORDcode
discord_get_guild_application_command(
    struct discord *client,
    u64snowflake application_id,
    u64snowflake guild_id,
    u64snowflake command_id,
    struct discord_ret_application_command *ret)
{
    struct discord_attributes attr = { 0 };

    CCORD_EXPECT(client, application_id != 0, CCORD_BAD_PARAMETER, "");
    CCORD_EXPECT(client, guild_id != 0, CCORD_BAD_PARAMETER, "");
    CCORD_EXPECT(client, command_id != 0, CCORD_BAD_PARAMETER, "");

    DISCORD_ATTR_INIT(attr, discord_application_command, ret, NULL);

    return discord_rest_run(&client->rest, &attr, NULL, HTTP_GET,
                            "/applications/%" PRIu64 "/guilds/%" PRIu64
                            "/commands/%" PRIu64,
                            application_id, guild_id, command_id);
}

CCORDcode
discord_edit_guild_application_command(
    struct discord *client,
    u64snowflake application_id,
    u64snowflake guild_id,
    u64snowflake command_id,
    struct discord_edit_guild_application_command *params,
    struct discord_ret_application_command *ret)
{
    struct discord_attributes attr = { 0 };
    struct ccord_szbuf body;
    char buf[4096];

    CCORD_EXPECT(client, application_id != 0, CCORD_BAD_PARAMETER, "");
    CCORD_EXPECT(client, guild_id != 0, CCORD_BAD_PARAMETER, "");
    CCORD_EXPECT(client, command_id != 0, CCORD_BAD_PARAMETER, "");

    body.size = discord_edit_guild_application_command_to_json(
        buf, sizeof(buf), params);
    body.start = buf;

    DISCORD_ATTR_INIT(attr, discord_application_command, ret, NULL);

    return discord_rest_run(&client->rest, &attr, &body, HTTP_PATCH,
                            "/applications/%" PRIu64 "/guilds/%" PRIu64
                            "/commands/%" PRIu64,
                            application_id, guild_id, command_id);
}

CCORDcode
discord_delete_guild_application_command(struct discord *client,
                                         u64snowflake application_id,
                                         u64snowflake guild_id,
                                         u64snowflake command_id,
                                         struct discord_ret *ret)
{
    struct discord_attributes attr = { 0 };

    CCORD_EXPECT(client, application_id != 0, CCORD_BAD_PARAMETER, "");
    CCORD_EXPECT(client, guild_id != 0, CCORD_BAD_PARAMETER, "");
    CCORD_EXPECT(client, command_id != 0, CCORD_BAD_PARAMETER, "");

    DISCORD_ATTR_BLANK_INIT(attr, ret, NULL);

    return discord_rest_run(&client->rest, &attr, NULL, HTTP_DELETE,
                            "/applications/%" PRIu64 "/guilds/%" PRIu64
                            "/commands/%" PRIu64,
                            application_id, guild_id, command_id);
}

CCORDcode
discord_bulk_overwrite_guild_application_commands(
    struct discord *client,
    u64snowflake application_id,
    u64snowflake guild_id,
    struct discord_bulk_overwrite_guild_application_commands *params,
    struct discord_ret_application_commands *ret)
{
    struct discord_attributes attr = { 0 };
    struct ccord_szbuf body;
    char buf[8192];

    CCORD_EXPECT(client, application_id != 0, CCORD_BAD_PARAMETER, "");
    CCORD_EXPECT(client, guild_id != 0, CCORD_BAD_PARAMETER, "");
    CCORD_EXPECT(client, params != NULL, CCORD_BAD_PARAMETER, "");

    body.size = discord_bulk_overwrite_guild_application_commands_to_json(
        buf, sizeof(buf), params);
    body.start = buf;

    DISCORD_ATTR_LIST_INIT(attr, discord_application_commands, ret, NULL);

    return discord_rest_run(&client->rest, &attr, &body, HTTP_PUT,
                            "/applications/%" PRIu64 "/guilds/%" PRIu64
                            "/commands",
                            application_id, guild_id);
}

CCORDcode
discord_get_guild_application_command_permissions(
    struct discord *client,
    u64snowflake application_id,
    u64snowflake guild_id,
    struct discord_ret_guild_application_command_permissions *ret)
{
    struct discord_attributes attr = { 0 };

    CCORD_EXPECT(client, application_id != 0, CCORD_BAD_PARAMETER, "");
    CCORD_EXPECT(client, guild_id != 0, CCORD_BAD_PARAMETER, "");

    DISCORD_ATTR_LIST_INIT(attr, discord_application_command_permissions, ret,
                           NULL);

    return discord_rest_run(&client->rest, &attr, NULL, HTTP_GET,
                            "/applications/%" PRIu64 "/guilds/%" PRIu64
                            "/commands/permissions",
                            application_id, guild_id);
}

CCORDcode
discord_get_application_command_permissions(
    struct discord *client,
    u64snowflake application_id,
    u64snowflake guild_id,
    u64snowflake command_id,
    struct discord_ret_application_command_permission *ret)
{
    struct discord_attributes attr = { 0 };

    CCORD_EXPECT(client, application_id != 0, CCORD_BAD_PARAMETER, "");
    CCORD_EXPECT(client, guild_id != 0, CCORD_BAD_PARAMETER, "");
    CCORD_EXPECT(client, command_id != 0, CCORD_BAD_PARAMETER, "");

    DISCORD_ATTR_INIT(attr, discord_application_command_permission, ret, NULL);

    return discord_rest_run(&client->rest, &attr, NULL, HTTP_GET,
                            "/applications/%" PRIu64 "/guilds/%" PRIu64
                            "/commands/%" PRIu64 "/permissions",
                            application_id, guild_id, command_id);
}
