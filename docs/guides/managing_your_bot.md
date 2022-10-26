# Managing Your Bot's Process

When it comes to running your bot, you have several options for running your bot in the background. Here's a selection of a few, 
these all apply to running Concord on UNIX-style systems.

## A Screen session
You can use GNU Screen to run a bot in the background, as a detached terminal session. First, SSH/telnet (remember: telnet is not secure 
over the open internet) into the machine that will run the bot, and install Screen. This can be done from your distribution package manager; 
the package is usually called `screen`. Once you've got it installed, run `screen -S sessionname` to start a Screen session called "sessionname."
Once that's started, run your bot. Once your bot has started and everything is working, press this key sequence: `Control-a`, then `d`. This will
"detach" the Screen session. Once you have detached, you will need to press `Control-l` (l as in lima) to clear the screen, or enter `clear`, as
the terminal will be "restored" and cause the shell prompt to be deposited in the top-right, over whatever text is already on the screen. Now,
when you're ready to re-attach, list your active Screen sessions with `screen -ls`. To reattach to a Screen session, use `screen -x sessionname`. 
While Screen isn't perfectly well-suited for serious management workloads, it's fine for small things. *Please note that you can apply essentially
the same sequence of actions, with different command arguments, to `tmux`.*

## BSD rc.d script
Here's an example of a file you could place in `/etc/rc.d/mybot`:
```
#!/bin/sh
#
# Run a Concord bot.
#

. /etc/rc.subr

name="mybot"
desc="My really cool Discord bot"

# Deposit mybot_enable="YES" in /etc/rc.conf, this here being the variable name:
rcvar="mybot_enable"

# This here is the location of the program.
command="/usr/local/libexec/${name}"

# This file holds the PID of the bot.
pidfile="/var/run/${name}.pid"

# This path would be hardcoded into your bot.
required_files="/usr/local/share/mybot/config.json"

extra_commands="reload"

load_rc_config $name
run_rc_command "$1"
```

Drop that into `/etc/rc.d`, substitute `mybot` for my the name of your bot, and start it up with `/etc/rc.d/mybot start` after you've enabled it
in `/etc/rc.conf` by putting `mybot_enable="YES"` somewhere in there. Make sure that the file is executable by the owner, owner group, and world;
else, it won't work. 

### systemd unit file
Drop this into your systemd unit file location. This can vary from system to system, but, it's usually located somewhere like
`/usr/lib/systemd/system`. Simply look for all of the `.service` files on your system, and you can find it. Make sure the file is executable.
Here's an example, name it `mybot.service` or whatever you'd like:

```
[Unit]
Description=My Concord bot

[Service]
ExecStart=/usr/local/bin/mybot

[Install]
WantedBy=multi-user.target
```

Once you've got the file set, run `systemctl daemon-reload`, then `systemctl start mybot.service`. The bot should start up.

### General considerations
With service files, the output from the console will be redirected *somewhere*, so, you will likely want to mess with the `logconf` functions
to redirect certain types of output to a dedicated logfile. The systemd journal will store output, but, you likely want to internally redirect
the output with `logconf` anyways. For BSD scripts, you will certainly want to manually tee the output, or adapt it to use the syslog facility
available on BSDs.
