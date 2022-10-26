# Setting up a bot

This quick guide will tell you how to set up a Discord bot application, and add it to a server.

### 1. Create the application
Head on over to [Discord Developers](https://discord.com/developers) and click "New Application." Enter a name, a description
(note that this will be your bot's "About Me" description), and save. 

### 2. Create the bot
Click on the "Bot" tab on the left (the one with the puzzle piece), and, click "Add Bot." You can check the box to make it
public, or leave it as private. You will probably want to set the "Privileged Gateway Intents" for the presence intent, 
server members intent, and the message content intent (especially if you wish to create a bot that uses "old-style" commands,
in other words, not slash interaction commands). Click "Reset Token" and grab the bot's token. Store that in a secure location,
you will need to enter it later.

### 3. Invite the bot
Click on OAuth2 on the left (with the wrench) and go to the URL generator subcategory. Check "bot" in the large checklist, 
and check the permissions that you want your bot to have. Note that "Administrator" permissions is a blanket-permission for all
further permissions. Once you've done that, click Copy, and paste the URL into a new browser tab. The Discord web app will open,
prompting you to approve the bot. 

### 4. Run the bot
Now that you've added your bot and you have its token, deposit that into `config.json` or your `discord_init` function, and start
the bot after you've compiled it. If all went well, the bot will come to life and should be fully functional.

#### Caveat for public and private bots
When a bot is said to be "public," the OAuth2 invite URL that you generated can be used by anyone on any server. When you set it
to "private," you can no longer do this; only you can add the bot to servers. Note that adding bots requires high Discord
guild permissions. Sadly, an invite link passed off to someone else won't work; you'll have to add it yourself.


