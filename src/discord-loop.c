#include <stdlib.h>
#include <errno.h>

#include "discord.h"
#include "discord-internal.h"
#include "cog-utils.h"

static void
discord_wake_timer_cb(struct discord *client, struct discord_timer *timer)
{
    (void)timer;
    if (client->wakeup_timer.cb) client->wakeup_timer.cb(client);
}

void
discord_set_next_wakeup(struct discord *client, int64_t delay)
{
    unsigned id = discord_internal_timer_ctl(
        client, &(struct discord_timer){
                    .id = client->wakeup_timer.id,
                    .on_tick = discord_wake_timer_cb,
                    .delay = delay,
                });
    client->wakeup_timer.id = id;
}

void
discord_set_on_wakeup(struct discord *client,
                      void (*callback)(struct discord *client))
{
    client->wakeup_timer.cb = callback;
    if (client->wakeup_timer.id) {
        discord_internal_timer_ctl(client,
                                   &(struct discord_timer){
                                       .id = client->wakeup_timer.id,
                                       .on_tick = discord_wake_timer_cb,
                                       .delay = -1,
                                   });
    }
}

void
discord_set_on_idle(struct discord *client,
                    void (*callback)(struct discord *client))
{
    client->on_idle = callback;
}

void
discord_set_on_cycle(struct discord *client,
                     void (*callback)(struct discord *client))
{
    client->on_cycle = callback;
}

#define BREAK_ON_FAIL(code, function)                                         \
    if (CCORD_OK != (code = function)) break

CCORDcode
discord_run(struct discord *client)
{
    struct discord_timers *const timers[] = { &client->timers.internal,
                                              &client->timers.user };
    int64_t now;
    CCORDcode code;

    while (1) {
        BREAK_ON_FAIL(code, discord_gateway_start(&client->gw));

        while (1) {
            int64_t poll_time = 0;

            now = (int64_t)discord_timestamp_us(client);

            if (!client->on_idle) {
                poll_time = discord_timers_get_next_trigger(
                    timers, sizeof timers / sizeof *timers, now, 60000000);
                if (poll_time % 1000 > 850)
                    poll_time += 1000 - (poll_time % 1000);
            }

            switch (io_poller_poll(client->io_poller, (int)(poll_time / 1000)))
            {
            case -1:
                // TODO: handle poll errno
                break;
            case 0:
                if (client->on_idle) {
                    client->on_idle(client);
                }
                else {
                    now = (int64_t)discord_timestamp_us(client);
                    int64_t sleep_time = discord_timers_get_next_trigger(
                        timers, sizeof timers / sizeof *timers, now, 1000);
                    if (sleep_time > 0 && sleep_time < 1000)
                        cog_sleep_us(sleep_time);
                }
                break;
            }

            if (client->on_cycle) client->on_cycle(client);

            for (unsigned i = 0; i < sizeof timers / sizeof *timers; i++)
                discord_timers_run(client, timers[i]);

            if (client->gw.session->status & DISCORD_SESSION_SHUTDOWN) break;

            BREAK_ON_FAIL(code, io_poller_perform(client->io_poller));

            discord_requestor_dispatch_responses(&client->rest.requestor);
        }

        logconf_info(&client->conf,
                     "Exits main gateway loop (code: %d, reason: %s)", code,
                     discord_strerror(code, client));

        /* stop all pending requests in case of connection shutdown */
        if (true == discord_gateway_end(&client->gw)) break;
    }

    return code;
}

#undef BREAK_ON_FAIL
#undef CALL_IO_POLLER_POLL
