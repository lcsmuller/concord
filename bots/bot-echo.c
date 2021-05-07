#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "discord.h"



void on_ready(struct discord *client, const struct discord_user *bot) {
  fprintf(stderr, "\n\nEcho-Bot succesfully connected to Discord as %s#%s!\n\n",
      bot->username, bot->discriminator);
}

void on_reaction_add(
    struct discord *client,
    const struct discord_user *bot,
    const uint64_t user_id, 
    const uint64_t channel_id, 
    const uint64_t message_id, 
    const uint64_t guild_id, 
    const struct discord_guild_member *member,
    const struct discord_emoji *emoji)
{ 
  // make sure bot doesn't echoes other bots
  if (member->user->bot) return;

  discord_create_reaction(client, channel_id, message_id, emoji->id, emoji->name);
}

void on_message_create(
    struct discord *client,
    const struct discord_user *bot,
    const struct discord_message *msg)
{
  // make sure bot doesn't echoes other bots
  if (msg->author->bot) return;

  struct discord_create_message_params params = { .content = msg->content };
  struct discord_message_reference msg_ref;
  discord_message_reference_init(&msg_ref);

  if (msg->referenced_message) {
    msg_ref.message_id = msg->referenced_message->id;
    msg_ref.channel_id = msg->channel_id;
    msg_ref.guild_id = msg->guild_id;
    params.message_reference = &msg_ref;
  }

  discord_create_message(client, msg->channel_id, &params, NULL);

  discord_message_reference_cleanup(&msg_ref);
}

void on_message_update(
    struct discord *client,
    const struct discord_user *bot,
    const struct discord_message *msg)
{
  struct discord_create_message_params params = {
    .content = "I see what you did there."
  };
  discord_create_message(client, msg->channel_id, &params, NULL);
}

void on_message_delete(
    struct discord *client,
    const struct discord_user *bot,
    const uint64_t id,
    const uint64_t channel_id,
    const uint64_t guild_id)
{
  struct discord_create_message_params params = {
    .content = "Did that message just disappear?"
  };
  discord_create_message(client, channel_id, &params, NULL);
}

void on_message_delete_bulk(
    struct discord *client,
    const struct discord_user *bot,
    const NTL_T(ja_u64) ids,
    const uint64_t channel_id,
    const uint64_t guild_id)
{
  char text[128];
  snprintf(text, sizeof(text), "Ouch! Where did those %zu messages go?", ntl_length((ntl_t)ids));
  struct discord_create_message_params params = { .content = text };
  discord_create_message(client, channel_id, &params, NULL);
}

int main(int argc, char *argv[])
{
  const char *config_file;
  if (argc > 1)
    config_file = argv[1];
  else
    config_file = "bot.config";

  discord_global_init();

  struct discord *client = discord_config_init(config_file);
  assert(NULL != client && "Couldn't initialize client");

  discord_set_on_ready(client, &on_ready);
  discord_set_on_message_create(client, &on_message_create);
  discord_set_on_message_update(client, &on_message_update);
  discord_set_on_message_delete(client, &on_message_delete);
  discord_set_on_message_reaction_add(client, &on_reaction_add);
  discord_set_on_message_delete_bulk(client, &on_message_delete_bulk);

  printf("\n\nThis bot demonstrates how easy it is to setup a bot that"
         " echoes user actions.\n"
         "1. Send a message in any chat\n"
         "2. Edit that message\n"
         "3. Delete that message\n"
         "4. Add a reaction to a message\n"
         "5. Have another bot bulk-delete messages\n"
         "\nTYPE ANY KEY TO START BOT\n");
  fgetc(stdin); // wait for input

  discord_run(client);

  discord_cleanup(client);

  discord_global_cleanup();
}


