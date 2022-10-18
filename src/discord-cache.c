#include <pthread.h>
#include <string.h>

#include "discord.h"
#include "discord-internal.h"
#include "anomap.h"

#define DISCORD_EPOCH 1420070400000

ANOMAP_DECLARE_COMPARE_FUNCTION(_cmp_sf, u64snowflake)

static int
_calculate_shard(u64snowflake guild_id, int total_shards)
{
    return (int)((guild_id >> 22) % (unsigned)total_shards);
}

struct _discord_shard_cache {
    pthread_mutex_t lock;
    bool valid;
    struct anomap *guild_map;
    struct anomap *msg_map;
};

struct _discord_cache_data {
    enum discord_cache_options options;
    struct _discord_shard_cache *caches;
    int total_shards;
    unsigned garbage_collection_timer;
};

static void
_discord_shard_cache_cleanup(struct discord *client,
                             struct _discord_shard_cache *cache)
{
    (void)client;
    pthread_mutex_lock(&cache->lock);
    anomap_clear(cache->guild_map);
    anomap_clear(cache->msg_map);
    pthread_mutex_unlock(&cache->lock);
}

#define EV_CB(name, data)                                                     \
    static void _on_##name(struct discord *client, const struct data *ev)

#define CACHE_BEGIN(DATA, CACHE, SHARD, GUILD_ID)                             \
    struct _discord_cache_data *const DATA = client->cache.data;              \
    const int SHARD = _calculate_shard(GUILD_ID, DATA->total_shards);         \
    struct _discord_shard_cache *const cache = &data->caches[SHARD];          \
    pthread_mutex_lock(&CACHE->lock)

#define CACHE_END(CACHE) pthread_mutex_unlock(&CACHE->lock)

EV_CB(ready, discord_ready)
{
    int shard = ev->shard ? ev->shard->array[0] : 0;
    struct _discord_cache_data *data = client->cache.data;
    struct _discord_shard_cache *cache = &data->caches[shard];
    pthread_mutex_lock(&cache->lock);
    cache->valid = true;
    pthread_mutex_unlock(&cache->lock);
}

static void
_on_shard_resumed(struct discord *client, const struct discord_identify *ev)
{
    int shard = ev->shard ? ev->shard->array[0] : 0;
    struct _discord_cache_data *data = client->cache.data;
    struct _discord_shard_cache *cache = &data->caches[shard];
    pthread_mutex_lock(&cache->lock);
    cache->valid = true;
    pthread_mutex_unlock(&cache->lock);
}

static void
_on_shard_reconnected(struct discord *client,
                      const struct discord_identify *ev)
{
    int shard = ev->shard ? ev->shard->array[0] : 0;
    struct _discord_cache_data *data = client->cache.data;
    struct _discord_shard_cache *cache = &data->caches[shard];
    _discord_shard_cache_cleanup(client, cache);
    pthread_mutex_lock(&cache->lock);
    cache->valid = true;
    pthread_mutex_unlock(&cache->lock);
}

static void
_on_shard_disconnected(struct discord *client,
                       const struct discord_identify *ev,
                       bool resumable)
{
    int shard = ev->shard ? ev->shard->array[0] : 0;
    struct _discord_cache_data *data = client->cache.data;
    struct _discord_shard_cache *cache = &data->caches[shard];
    if (!resumable) _discord_shard_cache_cleanup(client, cache);
    pthread_mutex_lock(&cache->lock);
    cache->valid = false;
    pthread_mutex_unlock(&cache->lock);
}

#define GUILD_BEGIN(guild)                                                    \
    struct discord_guild *guild = calloc(1, sizeof *guild);                   \
    memcpy(guild, ev, sizeof *guild);                                         \
    guild->channels = NULL;                                                   \
    guild->members = NULL;                                                    \
    guild->roles = NULL;                                                      \
    do {                                                                      \
        char buf[0x40000];                                                    \
        const size_t size = discord_guild_to_json(buf, sizeof buf, guild);    \
        memset(guild, 0, sizeof *guild);                                      \
        discord_guild_from_json(buf, size, guild);                            \
        discord_refcounter_add_internal(                                      \
            &client->refcounter, guild,                                       \
            (void (*)(void *))discord_guild_cleanup, true);                   \
    } while (0)

EV_CB(guild_create, discord_guild)
{
    CACHE_BEGIN(data, cache, shard, ev->id);
    GUILD_BEGIN(guild);
    enum anomap_operation op = anomap_insert;
    anomap_do(cache->guild_map, op, (u64snowflake *)&ev->id, &guild);
    CACHE_END(cache);
}

EV_CB(guild_update, discord_guild)
{
    CACHE_BEGIN(data, cache, shard, ev->id);
    GUILD_BEGIN(guild);
    struct anomap *map = cache->guild_map;
    enum anomap_operation op = anomap_upsert;
    anomap_do(map, op, (u64snowflake *)&ev->id, &guild);
    CACHE_END(cache);
}

EV_CB(guild_delete, discord_guild)
{
    CACHE_BEGIN(data, cache, shard, ev->id);
    struct discord_guild *guild = NULL;
    struct anomap *map = cache->guild_map;
    enum anomap_operation op = anomap_delete;
    anomap_do(map, op, (u64snowflake *)&ev->id, &guild);
    CACHE_END(cache);
}

// EV_CB(channel_create, discord_channel) {}
// EV_CB(channel_update, discord_channel) {}
// EV_CB(channel_delete, discord_channel) {}

// EV_CB(guild_role_create, discord_guild_role_create) {}
// EV_CB(guild_role_update, discord_guild_role_update) {}
// EV_CB(guild_role_delete, discord_guild_role_delete) {}

EV_CB(message_create, discord_message)
{
    CACHE_BEGIN(data, cache, shard, ev->guild_id);
    anomap_do(cache->msg_map, anomap_insert, (u64snowflake *)&ev->id, &ev);
    CACHE_END(cache);
}

EV_CB(message_update, discord_message)
{
    CACHE_BEGIN(data, cache, shard, ev->guild_id);
    anomap_do(cache->msg_map, anomap_upsert, (u64snowflake *)&ev->id, &ev);
    CACHE_END(cache);
}

EV_CB(message_delete, discord_message_delete)
{
    CACHE_BEGIN(data, cache, shard, ev->guild_id);
    anomap_do(cache->msg_map, anomap_delete, (u64snowflake *)&ev->id, NULL);
    CACHE_END(cache);
}

static void
_on_garbage_collection(struct discord *client, struct discord_timer *timer)
{
    (void)client;
    struct _discord_cache_data *data = timer->data;
    for (int i = 0; i < data->total_shards; i++) {
        struct _discord_shard_cache *const cache = &data->caches[i];
        pthread_mutex_lock(&cache->lock);
        { // DELETE MESSAGES
            u64snowflake delete_before =
                ((cog_timestamp_ms() - DISCORD_EPOCH) - 10 * 60 * 1000) << 22;
            size_t idx;
            anomap_index_of(cache->msg_map, &delete_before, &idx);
            if (idx--) anomap_delete_range(cache->msg_map, 0, idx, NULL, NULL);
        } // !DELETE MESSAGES
        pthread_mutex_unlock(&cache->lock);
    }
    timer->repeat = 1;
    timer->interval = 1000 * 60;
}

static void
_discord_cache_cleanup(struct discord *client)
{
    struct _discord_cache_data *data = client->cache.data;
    for (int i = 0; i < data->total_shards; i++) {
        struct _discord_shard_cache *cache = &data->caches[i];
        _discord_shard_cache_cleanup(client, cache);
        anomap_destroy(cache->guild_map);
        anomap_destroy(cache->msg_map);
        pthread_mutex_destroy(&cache->lock);
    }
    free(data->caches);
    discord_internal_timer_ctl(client,
                               &(struct discord_timer){
                                   .id = data->garbage_collection_timer,
                                   .flags = DISCORD_TIMER_DELETE,
                               });
    free(data);
}

static void
_on_guild_map_changed(struct anomap *map, struct anomap_item_changed *ev)
{
    struct discord_refcounter *rc = &((struct discord *)ev->data)->refcounter;
    (void)map;
    if (ev->op & (anomap_update | anomap_delete))
        discord_refcounter_decr(rc, *(void **)ev->val.prev);
}

static void
_on_map_changed(struct anomap *map, struct anomap_item_changed *ev)
{
    struct discord_refcounter *rc = &((struct discord *)ev->data)->refcounter;
    (void)map;
    if (ev->op & (anomap_delete | anomap_update))
        discord_refcounter_decr(rc, *(void **)ev->val.prev);
    if (ev->op & anomap_upsert)
        discord_refcounter_incr(rc, *(void **)ev->val.now);
}

#define ASSIGN_CB(ev, cb) client->gw.cbs[0][ev] = (discord_ev_event)_on_##cb

void
discord_cache_enable(struct discord *client,
                     enum discord_cache_options options)
{
    struct _discord_cache_data *data = client->cache.data;
    if (!data) {
        client->cache.cleanup = _discord_cache_cleanup;
        data = client->cache.data = calloc(1, sizeof *data);

        size_t nshards = (size_t)(data->total_shards = 1);
        data->caches = calloc(nshards, sizeof *data->caches);
        for (int i = 0; i < data->total_shards; i++) {
            struct _discord_shard_cache *cache = &data->caches[i];
            pthread_mutex_init(&cache->lock, NULL);
            const size_t sf_sz = sizeof(u64snowflake);
            cache->guild_map = anomap_create(sf_sz, sizeof(void *), _cmp_sf);
            anomap_set_on_item_changed(cache->guild_map, _on_guild_map_changed,
                                       client);
            cache->msg_map = anomap_create(sf_sz, sizeof(void *), _cmp_sf);
            anomap_set_on_item_changed(cache->msg_map, _on_map_changed,
                                       client);
        }
        data->garbage_collection_timer = discord_internal_timer(
            client, _on_garbage_collection, NULL, data, 0);
    }
    data->options |= options;
    ASSIGN_CB(DISCORD_EV_READY, ready);
    client->cache.on_shard_resumed = _on_shard_resumed;
    client->cache.on_shard_reconnected = _on_shard_reconnected;
    client->cache.on_shard_disconnected = _on_shard_disconnected;

    if (options & DISCORD_CACHE_GUILDS) {
        discord_add_intents(client, DISCORD_GATEWAY_GUILDS);
        ASSIGN_CB(DISCORD_EV_GUILD_CREATE, guild_create);
        ASSIGN_CB(DISCORD_EV_GUILD_UPDATE, guild_update);
        ASSIGN_CB(DISCORD_EV_GUILD_DELETE, guild_delete);

        // ASSIGN_CB(DISCORD_EV_CHANNEL_CREATE, channel_create);
        // ASSIGN_CB(DISCORD_EV_CHANNEL_UPDATE, channel_update);
        // ASSIGN_CB(DISCORD_EV_CHANNEL_DELETE, channel_delete);

        // ASSIGN_CB(DISCORD_EV_GUILD_ROLE_CREATE, guild_role_create);
        // ASSIGN_CB(DISCORD_EV_GUILD_ROLE_UPDATE, guild_role_update);
        // ASSIGN_CB(DISCORD_EV_GUILD_ROLE_DELETE, guild_role_delete);
    }

    if (options & DISCORD_CACHE_MESSAGES) {
        ASSIGN_CB(DISCORD_EV_MESSAGE_CREATE, message_create);
        ASSIGN_CB(DISCORD_EV_MESSAGE_UPDATE, message_update);
        ASSIGN_CB(DISCORD_EV_MESSAGE_DELETE, message_delete);
    }
}

const struct discord_message *
discord_cache_get_channel_message(struct discord *client,
                                  u64snowflake channel_id,
                                  u64snowflake message_id)
{
    if (!client->cache.data) return NULL;
    struct _discord_cache_data *data = client->cache.data;
    for (int i = 0; i < data->total_shards; i++) {
        struct _discord_shard_cache *cache = &data->caches[i];
        struct discord_message *message = NULL;
        pthread_mutex_lock(&cache->lock);
        anomap_do(cache->msg_map, anomap_getval, &message_id, &message);
        const bool found = message;
        const bool valid = cache->valid;
        if (found && message->channel_id != channel_id) message = NULL;
        if (message && valid) (void)discord_claim(client, message);
        pthread_mutex_unlock(&cache->lock);
        if (found) return valid ? message : NULL;
    }
    return NULL;
}

const struct discord_guild *
discord_cache_get_guild(struct discord *client, u64snowflake guild_id)
{
    if (!client->cache.data) return NULL;
    struct _discord_cache_data *data = client->cache.data;
    struct _discord_shard_cache *cache =
        &data->caches[_calculate_shard(guild_id, data->total_shards)];
    struct discord_guild *guild = NULL;
    pthread_mutex_lock(&cache->lock);
    anomap_do(cache->guild_map, anomap_getval, &guild_id, &guild);
    const bool valid = cache->valid;
    if (guild && valid) (void)discord_claim(client, guild);
    pthread_mutex_unlock(&cache->lock);
    if (guild && valid) return guild;
    return NULL;
}
