#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "discord.h"
#include "discord-internal.h"

ANOMAP_DECLARE_COMPARE_FUNCTION(_cmp_ptr, void *)

struct _discord_refvalue {
    const char *const name;
    /**
     * cleanup for when `data` is no longer needed
     * @note this only has to be assigned once, it is automatically called once
     *      `data` is no longer referenced by any callback */
    union {
        void (*client)(struct discord *client, void *data);
        void (*internal)(void *data);
    } cleanup;
    /** how many times this resource has been discord_claim() 'd */
    int claims;
    /**
     * how many references the internal client is holding
     */
    short visits;
    /** whether `data` cleanup should also be followed by a free() */
    bool should_free;
    /** whether cleanup expects a client parameter */
    bool expects_client;
};

static void
_on_ref_map_changed(const struct anomap_item_changed *ev)
{
    struct discord_refcounter *rc = ev->data;
    if (ev->op & anomap_delete) {
        struct _discord_refvalue *val = ev->val.prev;
        logconf_trace(&rc->conf, "deleting %s %p", val->name,
                      *(void **)ev->key);
        if (val->cleanup.client) {
            if (val->expects_client)
                val->cleanup.client(CLIENT(ev->data, refcounter),
                                    *(void **)ev->key);
            else
                val->cleanup.internal(*(void **)ev->key);
        }
        if (val->should_free) free(*(void **)ev->key);
    }
}

void
discord_refcounter_init(struct discord_refcounter *rc, struct logconf *conf)
{
    logconf_branch(&rc->conf, conf, "DISCORD_REFCOUNT");
    for (int i = 0; i < 16; i++) {
        rc->maps[i] =
            anomap_create(sizeof(void *), sizeof(struct _discord_refvalue),
                          _cmp_ptr, anomap_direct_access);
        anomap_set_on_item_changed(rc->maps[i], _on_ref_map_changed, rc);
        pthread_mutex_init(&rc->locks[i], NULL);
    }
}

void
discord_refcounter_cleanup(struct discord_refcounter *rc)
{
    for (int i = 0; i < 16; i++) {
        anomap_destroy(rc->maps[i]);
        pthread_mutex_destroy(&rc->locks[i]);
    }
}

static size_t
_get_map_index(const void *data)
{
    return (((uintptr_t)data) >> 6) & 0xF;
}

static CCORDcode
_discord_refcounter_mod_no_lock(struct discord_refcounter *rc,
                                const void *data,
                                short visits,
                                int claims)
{
    struct anomap *map = rc->maps[_get_map_index(data)];
    size_t index;
    bool found = anomap_index_of(map, &data, &index);
    struct _discord_refvalue *val;
    if (!found) {
        logconf_trace(&rc->conf, "%p not found", data);
        return CCORD_RESOURCE_UNAVAILABLE;
    }

    val = anomap_direct_val_at_index(map, index);
    static const char *operation_name[] = { "decrementing", "incrementing" };

    if (0 != visits)
        logconf_trace(&rc->conf, "visits %p %s by %i to %i", data,
                      operation_name[visits > 0],
                      visits > 0 ? visits : -visits, val->visits += visits);

    if (0 != claims)
        logconf_trace(&rc->conf, "claims %p %s by %i to %i", data,
                      operation_name[claims > 0],
                      claims > 0 ? claims : -claims, val->claims += claims);

    if (0 <= val->visits && 0 <= val->claims) {
        if (0 == val->visits && 0 == val->claims)
            anomap_do(map, anomap_delete, &data, NULL);
        return CCORD_OK;
    }

    if (0 > val->visits)
        logconf_error(&rc->conf, "visits for %p below 0", data);

    if (0 > val->claims)
        logconf_error(&rc->conf, "claims for %p below 0", data);

    return CCORD_RESOURCE_UNAVAILABLE;
}

static CCORDcode
_discord_refcounter_mod(struct discord_refcounter *rc,
                        const void *data,
                        short visits,
                        int claims)
{
    size_t arrayi = _get_map_index(data);
    pthread_mutex_lock(&rc->locks[arrayi]);
    CCORDcode code = _discord_refcounter_mod_no_lock(rc, data, visits, claims);
    pthread_mutex_unlock(&rc->locks[arrayi]);
    return code;
}

CCORDcode
discord_refcounter_claim(struct discord_refcounter *rc, const void *data)
{
    return _discord_refcounter_mod(rc, data, 0, 1);
}

CCORDcode
discord_refcounter_unclaim(struct discord_refcounter *rc, void *data)
{
    return _discord_refcounter_mod(rc, data, 0, -1);
}

void
__discord_refcounter_add_internal(struct discord_refcounter *rc,
                                  const char *name,
                                  void *data,
                                  void (*cleanup)(void *data),
                                  bool should_free)
{
    size_t arrayi = _get_map_index(data);
    struct anomap *map = rc->maps[arrayi];
    pthread_mutex_lock(&rc->locks[arrayi]);
    struct _discord_refvalue val = {
        .name = name,
        .expects_client = false,
        .cleanup.internal = cleanup,
        .should_free = should_free,
        .visits = 1,
    };
    anomap_operation result = anomap_do(map, anomap_insert, &data, &val);
    logconf_trace(&rc->conf, "Adding concord's internal resource %s %p %s",
                  name, data,
                  result & anomap_insert ? "successful" : "failed");
    pthread_mutex_unlock(&rc->locks[arrayi]);
}

void
__discord_refcounter_add_client(struct discord_refcounter *rc,
                                const char *name,
                                void *data,
                                void (*cleanup)(struct discord *client,
                                                void *data),
                                bool should_free)
{
    size_t arrayi = _get_map_index(data);
    struct anomap *map = rc->maps[arrayi];
    pthread_mutex_lock(&rc->locks[arrayi]);
    struct _discord_refvalue val = {
        .name = name,
        .expects_client = true,
        .cleanup.client = cleanup,
        .should_free = should_free,
        .visits = 1,
    };
    anomap_operation result = anomap_do(map, anomap_insert, &data, &val);
    logconf_trace(&rc->conf, "Adding user's custom resource %s %p %s", name,
                  data, result & anomap_insert ? "successful" : "failed");
    pthread_mutex_unlock(&rc->locks[arrayi]);
}

CCORDcode
discord_refcounter_incr(struct discord_refcounter *rc, void *data)
{
    return _discord_refcounter_mod(rc, data, 1, 0);
}

CCORDcode
discord_refcounter_decr(struct discord_refcounter *rc, void *data)
{
    return _discord_refcounter_mod(rc, data, -1, 0);
}
