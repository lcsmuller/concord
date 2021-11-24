#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "discord.h"
#include "cee-utils.h" /* cee_timestamp_ms() */

void on_ready(struct discord *client, const struct discord_user *bot)
{
  log_info("Presence-Bot succesfully connected to Discord as %s#%s!",
           bot->username, bot->discriminator);
}

int main(int argc, char *argv[])
{
  const char *config_file;
  if (argc > 1)
    config_file = argv[1];
  else
    config_file = "../config.json";

  discord_global_init();

  struct discord *client = discord_config_init(config_file);
  assert(NULL != client && "Couldn't initialize client");

  printf("\n\nThis bot demonstrates how easy it is to set the bot presence.\n"
         "1. Login\n"
         "2. Check the bot status\n"
         "\nTYPE ANY KEY TO START BOT\n");
  fgetc(stdin); // wait for input

  /* custom presence */
  discord_set_presence(
    client, &(struct discord_presence_status){
              .activities =
                (struct discord_activity *[]){
                  &(struct discord_activity){ .name = "with Orca",
                                              .type = DISCORD_ACTIVITY_GAME,
                                              .details = "Fixing some bugs" },
                  NULL // END OF ACTIVITY ARRAY
                },
              .status = "idle",
              .afk = false,
              .since = cee_timestamp_ms() });

  discord_run(client);

  discord_cleanup(client);

  discord_global_cleanup();
}