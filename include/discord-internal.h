/**
 * @file discord-internal.h
 * @ingroup DiscordInternal
 * @author Cogmasters
 * @brief Internal functions and datatypes
 */

#ifndef DISCORD_INTERNAL_H
#define DISCORD_INTERNAL_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <pthread.h>

#define JSONB_HEADER
#include "json-build.h"
#define JSMN_STRICT
#define JSMN_HEADER
#include "jsmn.h"
#include "jsmn-find.h"

#include "logconf.h"
#include "user-agent.h"
#include "websockets.h"
#include "cog-utils.h"
#include "io_poller.h"
#include "queue.h"
#include "priority_queue.h"
#include "attributes.h"
#include "anomap.h"

/** @brief Return 1 if string isn't considered empty */
#define NOT_EMPTY_STR(str) ((str) && *(str))
/**
 * @brief Get container `type` from a field `ptr`
 *
 * @param[in] ptr the field contained in `type`
 * @param[in] type the container datatype
 * @param[in] path the path to the field from the container POV
 */
#define CONTAINEROF(ptr, type, path)                                          \
    ((type *)((char *)(ptr)-offsetof(type, path)))

/** @defgroup DiscordInternal Internal implementation details
 * @brief Documentation useful when developing or debugging Concord itself
 *  @{ */

/** @brief dup shutdown fd to listen for ccord_shutdown_async() */
int discord_dup_shutdown_fd(void);

/** @brief Get client from its nested field */
#define CLIENT(ptr, path) CONTAINEROF(ptr, struct discord, path)

/**
 * @brief log and return `code` if `expect` condition is false
 *
 * @param[in] client the Discord client
 * @param[in] expect the expected outcome
 * @param[in] code return CCORDcode error code
 * @param[in] reason for return
 * @return the provided @ref CCORDcode `code` parameter
 */
#define CCORD_EXPECT(client, expect, code, reason)                            \
    do {                                                                      \
        if (!(expect)) {                                                      \
            logconf_error(&(client)->conf, "Expected: " #expect ": " reason); \
            return code;                                                      \
        }                                                                     \
    } while (0)

/**
 * @brief Shortcut for checking OOB-write attempts
 * @note unsigned values are expected
 *
 * @param[in] nbytes amount of bytes to be written
 * @param[in] destsz size of dest in bytes
 */
#define ASSERT_NOT_OOB(nbytes, destsz)                                        \
    ASSERT_S((size_t)nbytes < (size_t)destsz, "Out of bounds write attempt");

/** URL endpoint threshold length */
#define DISCORD_ENDPT_LEN 512
/** Route's unique key threshold length */
#define DISCORD_ROUTE_LEN 256

/** @defgroup DiscordInternalTimer Timer API
 * @brief Callback scheduling API
 *  @{ */

struct discord_timers {
    priority_queue *q;
    struct io_poller *io;
    struct {
        bool is_active;
        pthread_t thread;
        struct discord_timer *timer;
        bool skip_update_phase;
    } active;
    pthread_mutex_t lock;
    pthread_cond_t cond;
};

/**
 * @brief Prepare timers for usage
 *
 * @param timers the 'struct discord_timers' to init
 */
void discord_timers_init(struct discord_timers *timers, struct io_poller *io);

/**
 * @brief Cleanup timers and call cancel any running ones
 *
 * @param client the client created with discord_init()
 * @param timers the 'struct discord_timers' to cleanup
 */
void discord_timers_cleanup(struct discord *client,
                            struct discord_timers *timers);

/**
 * @brief Get earliest trigger time from a group of timers
 *
 * @param timers array of timers
 * @param n number of timers in array
 * @param now current time
 * @param max_time max time to allowed
 * @return time in microseconds until next timer, or max
 */
int64_t discord_timers_get_next_trigger(struct discord_timers *const timers[],
                                        size_t n,
                                        int64_t now,
                                        int64_t max_time);

/**
 * @brief Run all timers that are due
 *
 * @param client the client created with discord_init()
 * @param timers the timers to run
 */
void discord_timers_run(struct discord *client, struct discord_timers *timers);

/**
 * @brief Modifies or creates a timer
 *
 * @param client the client created with discord_init()
 * @param timers the timer group to perform this operation on
 * @param timer the timer that should be modified
 * @return the id of the timer
 */
unsigned _discord_timer_ctl(struct discord *client,
                            struct discord_timers *timers,
                            struct discord_timer *timer);

/**
 * @brief Modifies or creates a timer
 *
 * @param client the client created with discord_init()
 * @param timer the timer that should be modified
 * @return the id of the timer
 */
unsigned discord_internal_timer_ctl(struct discord *client,
                                    struct discord_timer *timer);

/**
 * @brief Creates a one shot timer that automatically deletes itself upon
 *      completion
 *
 * @param client the client created with discord_init()
 * @param on_tick_cb (nullable) the callback that should be called when timer
 * triggers
 * @param on_status_changed_cb (nullable) the callback for status updates
 * timer->flags will have: DISCORD_TIMER_CANCELED, and DISCORD_TIMER_DELETE
 * @param data user data
 * @param delay delay before timer should start in milliseconds
 * @return the id of the timer
 */
unsigned discord_internal_timer(struct discord *client,
                                discord_ev_timer on_tick_cb,
                                discord_ev_timer on_status_changed_cb,
                                void *data,
                                int64_t delay);

/** @} DiscordInternalTimer */

/** @defgroup DiscordInternalREST REST API
 * @brief Wrapper to the Discord REST API
 *  @{ */

/** @defgroup DiscordInternalRESTRequest Request's handling
 * @brief Store, manage and dispatch individual requests
 *  @{ */

/** @defgroup DiscordInternalRESTRequestRatelimit Ratelimiting
 * @brief Enforce ratelimiting per the official Discord Documentation
 *  @{ */

/**
 * @brief Value assigned to @ref discord_bucket `busy_req` field in case
 *      it's being timed-out
 */
#define DISCORD_BUCKET_TIMEOUT (void *)(0xf)

/**
 * @brief The ratelimiter struct for handling ratelimiting
 * @note this struct **SHOULD** only be handled from the `REST` manager thread
 */
struct discord_ratelimiter {
    /** `DISCORD_RATELIMIT` logging module */
    struct logconf conf;
    /** amount of bucket's routes discovered */
    int length;
    /** route's cap before increase */
    int capacity;
    /**
     * routes matched to individual buckets
     * @note datatype declared at discord-rest_ratelimit.c
     */
    struct _discord_route *routes;
    /** singleton bucket for requests that haven't been matched to a
     *      known or new bucket (i.e first time running the request) */
    struct discord_bucket *null;
    /** singleton bucket for requests that are not part of any known
     *      ratelimiting group */
    struct discord_bucket *miss;

    /* client-wide global ratelimiting */
    u64unix_ms *global_wait_tstamp;

    /** bucket queues */
    struct {
        /** buckets that are currently pending (have pending requests) */
        QUEUE(struct discord_bucket) pending;
    } queues;
};

/**
 * @brief Initialize ratelimiter handle
 *
 * A hashtable shall be used for storage and retrieval of discovered buckets
 * @param rl the ratelimiter handle to be initialized
 * @param conf pointer to @ref discord_rest logging module
 */
void discord_ratelimiter_init(struct discord_ratelimiter *rl,
                              struct logconf *conf);

/**
 * @brief Cleanup all buckets that have been discovered
 *
 * @note pending requests will be moved to `rest.queues->recycling`
 * @param rl the handle initialized with discord_ratelimiter_init()
 */
void discord_ratelimiter_cleanup(struct discord_ratelimiter *rl);

/**
 * @brief Build unique key formed from the HTTP method and endpoint
 * @see https://discord.com/developers/docs/topics/rate-limits
 *
 * @param[in] method the request method
 * @param[out] key unique key for matching to buckets
 * @param[in] endpoint_fmt the printf-like endpoint formatting string
 * @param[in] args variadic arguments matched to `endpoint_fmt`
 */
void discord_ratelimiter_build_key(enum http_method method,
                                   char key[DISCORD_ROUTE_LEN],
                                   const char endpoint_fmt[],
                                   va_list args);

/**
 * @brief Update the bucket with response header data
 *
 * @param rl the handle initialized with discord_ratelimiter_init()
 * @param bucket NULL when bucket is first discovered
 * @param key obtained from discord_ratelimiter_build_key()
 * @param info informational struct containing details on the current transfer
 * @note If the bucket was just discovered it will be created here.
 */
void discord_ratelimiter_build(struct discord_ratelimiter *rl,
                               struct discord_bucket *bucket,
                               const char key[],
                               struct ua_info *info);

/**
 * @brief Update global ratelimiting value
 * @todo check if all pending buckets must be unset
 *
 * @param rl the handle initialized with discord_ratelimiter_init()
 * @param bucket bucket that received the global ratelimiting notice
 * @param wait_ms the amount of time that all buckets should wait for
 */
void discord_ratelimiter_set_global_timeout(struct discord_ratelimiter *rl,
                                            struct discord_bucket *bucket,
                                            u64unix_ms wait_ms);

/** @brief The Discord bucket for handling per-group ratelimits */
struct discord_bucket {
    /** the hash associated with the bucket's ratelimiting group */
    char hash[64];
    /** maximum connections this bucket can handle before ratelimit */
    long limit;
    /** connections this bucket can do before pending for cooldown */
    long remaining;
    /** timestamp of when cooldown timer resets */
    u64unix_ms reset_tstamp;

    /**
     * pointer to this bucket's currently busy request
     * @note @ref DISCORD_BUCKET_TIMEOUT if bucket is being ratelimited
     */
    struct discord_request *busy_req;

    /** request queues */
    struct {
        /** next requests queue */
        QUEUE(struct discord_request) next;
    } queues;
    /** entry for @ref discord_ratelimiter pending buckets queue */
    QUEUE entry;
};

/**
 * @brief Set bucket timeout
 *
 * @param bucket the bucket to be checked for time out
 * @param wait_ms how long the bucket should wait for
 */
void discord_bucket_set_timeout(struct discord_bucket *bucket,
                                u64unix_ms wait_ms);

/**
 * @brief Get a `struct discord_bucket` assigned to `key`
 *
 * @param rl the handle initialized with discord_ratelimiter_init()
 * @param key obtained from discord_ratelimiter_build_key()
 * @return bucket matched to `key`
 */
struct discord_bucket *discord_bucket_get(struct discord_ratelimiter *rl,
                                          const char key[]);

/**
 * @brief Insert into bucket's next requests queue
 *
 * @param rl the handle initialized with discord_ratelimiter_init()
 * @param b the bucket to insert the request to
 * @param req the request to be inserted to bucket
 * @param high_priority if high priority then request shall be prioritized over
 *      already enqueued requests
 */
void discord_bucket_insert(struct discord_ratelimiter *rl,
                           struct discord_bucket *b,
                           struct discord_request *req,
                           bool high_priority);

/**
 * @brief Iterate and select next requests
 * @note discord_bucket_unselect() must be called once bucket's current request
 *      is done and its next one should be selected
 *
 * @param rl the handle initialized with discord_ratelimiter_init()
 * @param data user arbitrary data
 * @param iter the user callback to be called per bucket
 */
void discord_bucket_request_selector(
    struct discord_ratelimiter *rl,
    void *data,
    void (*iter)(void *data, struct discord_request *req));

/**
 * @brief Unselect a request provided at discord_ratelimiter_request_selector()
 * @note counterpart to discord_ratelimiter_request_selector()
 *
 * @param rl the handle initialized with discord_ratelimiter_init()
 * @param b the request's bucket
 * @param req the request to unslect
 */
void discord_bucket_request_unselect(struct discord_ratelimiter *rl,
                                     struct discord_bucket *b,
                                     struct discord_request *req);

/** @} DiscordInternalRESTRequestRatelimit */

/** @brief Generic request dispatcher */
struct discord_ret_dispatch {
    DISCORD_RET_DEFAULT_FIELDS;
    /** `true` if may receive a datatype from response */
    bool has_type;

    /**
     * optional callback to be executed on a successful request
     * @todo should be cast to the original callback signature before calling,
     *      otherwise its UB
     */
    union {
        void (*typed)(struct discord *client,
                      struct discord_response *resp,
                      const void *ret);
        void (*typeless)(struct discord *client,
                         struct discord_response *resp);
    } done;

    /** if an address is provided, then request will block the thread and
     * perform on-spot. On success the response object will be written to
     * the address. */
    void *sync;
};

/** @brief Attributes of response datatype */
struct discord_ret_response {
    /** pointer to datatype */
    void *data;
    /** size of datatype in bytes */
    size_t size;
    /** initializer function for datatype fields */
    void (*init)(void *data);
    /** populate datatype with JSON values */
    size_t (*from_json)(const char *json, size_t len, void *data);
    /** cleanup function for datatype */
    void (*cleanup)(void *data);
};

/**
 * @brief Macro containing @ref discord_attributes fields
 * @note this exists for @ref discord_request alignment purposes
 */
#define DISCORD_ATTRIBUTES_FIELDS                                             \
    /** attributes set by client for request dispatch behavior */             \
    struct discord_ret_dispatch dispatch;                                     \
    /** information for parsing response into a datatype (if possible) */     \
    struct discord_ret_response response;                                     \
    /** if @ref HTTP_MIMEPOST provide attachments for file transfer */        \
    struct discord_attachments attachments;                                   \
    /** indicated reason to why the action was taken @note when used at       \
     *      @ref discord_request buffer is kept and reused */                 \
    char *reason

/** @brief Request to be performed */
struct discord_attributes {
    DISCORD_ATTRIBUTES_FIELDS;
};

/**
 * @brief Individual requests that are scheduled to run asynchronously
 * @note this struct **SHOULD NOT** be handled from the `REST` manager thread
 * @note its fields are aligned with @ref discord_attributes
 *      (see @ref DISCORD_ATTRIBUTES_FIELDS)
 */
struct discord_request {
    DISCORD_ATTRIBUTES_FIELDS;

    /** the request's bucket */
    struct discord_bucket *b;
    /** request body handle @note buffer is kept and reused */
    struct ccord_szbuf_reusable body;
    /** the request's http method */
    enum http_method method;
    /** the request's endpoint */
    char endpoint[DISCORD_ENDPT_LEN];
    /** the request bucket's key */
    char key[DISCORD_ROUTE_LEN];
    /** the connection handler assigned */
    struct ua_conn *conn;
    /** request's status code */
    CCORDcode code;
    /** current retry attempt (stop at rest->retry_limit) */
    int retry_attempt;
    /** synchronize synchronous requests */
    pthread_cond_t *cond;
    /** entry for @ref discord_ratelimiter and @ref discord_bucket queues */
    QUEUE entry;
};

/** @brief The handle used for handling asynchronous requests */
struct discord_requestor {
    /** `DISCORD_REQUEST` logging module */
    struct logconf conf;
    /** the user agent handle for performing requests */
    struct user_agent *ua;
    /** curl_multi handle for performing asynchronous requests */
    CURLM *mhandle;
    /** enforce Discord's ratelimiting for requests */
    struct discord_ratelimiter ratelimiter;

    /** max amount of retries before a failed request gives up */
    int retry_limit;

    /** request queues */
    struct {
        /** requests for recycling */
        QUEUE(struct discord_request) recycling;
        /** pending requests waiting to be assigned to a bucket */
        QUEUE(struct discord_request) pending;
        /**
         * finished requests that are done performing and waiting for
         *      their callbacks to be called from the main thread
         */
        QUEUE(struct discord_request) finished;
    } *queues;

    /** queue locks */
    struct {
        /** recycling queue lock */
        pthread_mutex_t recycling;
        /** pending queue lock */
        pthread_mutex_t pending;
        /** finished queue lock */
        pthread_mutex_t finished;
    } *qlocks;
};

/**
 * @brief Initialize the request handler
 *
 * This shall initialize a `CURLM` multi handle for performing requests
 *      asynchronously, and a queue for storing individual requests
 * @param rqtor the requestor handle to be initialized
 * @param conf pointer to @ref discord_rest logging module
 * @param token the bot token
 */
void discord_requestor_init(struct discord_requestor *rqtor,
                            struct logconf *conf,
                            const char token[]);

/**
 * @brief Free the request handler
 *
 * @param rqtor the handle initialized with discord_requestor_init()
 */
void discord_requestor_cleanup(struct discord_requestor *rqtor);

/**
 * @brief Check for and start pending bucket's requests
 *
 * @param rqtor the handle initialized with discord_requestor_init()
 * @CCORD_return
 */
CCORDcode discord_requestor_start_pending(struct discord_requestor *rqtor);

/**
 * @brief Poll for request's completion
 *
 * @param rqtor the handle initialized with discord_requestor_init()
 * @CCORD_return
 */
CCORDcode discord_requestor_info_read(struct discord_requestor *rqtor);

/**
 * @brief Run pending callbacks from completed requests
 *
 * @param req the request containing preliminary information for its dispatch
 */
void discord_requestor_dispatch_responses(struct discord_requestor *rqtor);

/**
 * @brief Mark request as canceled and move it to the recycling queue
 *
 * @param rqtor the requestor handle initialized with discord_requestor_init()
 * @param req the on-going request to be canceled
 */
void discord_request_cancel(struct discord_requestor *rqtor,
                            struct discord_request *req);

/**
 * @brief Begin a new request
 *
 * The returned request automatically be performed from the `REST` thread
 * @param rqtor the requestor handle initialized with discord_requestor_init()
 * @param req the request containing preliminary information for its dispatch
 * and response's parsing
 * @param body the request's body
 * @param method the request's HTTP method
 * @param endpoint the request's endpoint
 * @param key the request bucket's group for ratelimiting
 * @CCORD_return
 */
CCORDcode discord_request_begin(struct discord_requestor *rqtor,
                                struct discord_attributes *req,
                                struct ccord_szbuf *body,
                                enum http_method method,
                                char endpoint[DISCORD_ENDPT_LEN],
                                char key[DISCORD_ROUTE_LEN]);

/** @} DiscordInternalRESTRequest */

/**
 * @brief The handle used for interfacing with Discord's REST API
 *
 * This handle will manage the special REST thread where requests are performed
 *      in
 */
struct discord_rest {
    /** `DISCORD_HTTP` or `DISCORD_WEBHOOK` logging module */
    struct logconf conf;
    /** the requests handler */
    struct discord_requestor requestor;
    /** the timer queue for the rest thread */
    struct discord_timers timers;
    /** poller for REST requests */
    struct io_poller *io_poller;
    /** threadpool for managing the REST thread */
    struct threadpool_t *tpool;
};

/**
 * @brief Initialize an REST handle
 *
 * Structure used for interfacing with the Discord's REST API
 * @param rest the REST handle to be initialized
 * @param conf pointer to @ref discord logging module
 * @param token the bot token
 */
void discord_rest_init(struct discord_rest *rest,
                       struct logconf *conf,
                       const char token[]);

/**
 * @brief Free an REST handle
 *
 * @param rest the handle initialized with discord_rest_init()
 */
void discord_rest_cleanup(struct discord_rest *rest);

/**
 * @brief Perform a request to Discord
 *
 * This functions is a selector over discord_rest_run() or
 *        discord_rest_run_requestor()
 * @param rest the handle initialized with discord_rest_init()
 * @param req return object of request
 * @param body the body sent for methods that require (ex: post), leave as
 *        null if unecessary
 * @param method the method in opcode format of the request being sent
 * @param endpoint_fmt the printf-like endpoint formatting string
 * @CCORD_return
 * @note if sync is set then this function will block the thread and perform it
 *              immediately
 */
CCORDcode discord_rest_run(struct discord_rest *rest,
                           struct discord_attributes *req,
                           struct ccord_szbuf *body,
                           enum http_method method,
                           char endpoint_fmt[],
                           ...) PRINTF_LIKE(5, 6);

/**
 * @brief Stop all bucket's on-going, pending and timed-out requests
 *
 * The requests will be moved over to client's 'queues->recycling' queue
 * @param rest the handle initialized with discord_rest_init()
 */
void discord_rest_stop_buckets(struct discord_rest *rest);

/** @} DiscordInternalREST */

/** @defgroup DiscordInternalGateway WebSockets API
 * @brief Wrapper to the Discord Gateway API
 *  @{ */

/** @defgroup DiscordInternalGatewaySessionStatus Client's session status
 * @brief Client's session status
 *  @{ */
/** client is currently offline */
#define DISCORD_SESSION_OFFLINE 0u
/** client will attempt to resume session after reconnect */
#define DISCORD_SESSION_RESUMABLE 1u << 0
/** client in the process of being shutdown */
#define DISCORD_SESSION_SHUTDOWN 1u << 1
/** @} DiscordInternalGatewaySessionStatus */

/** @brief The handle for storing the Discord Gateway session */
struct discord_gateway_session {
    /** whether client is ready to start sending/receiving events */
    bool is_ready;
    /** session id for resuming lost connections */
    char id[64];
    /** amount of shards being used by this session */
    int shards;
    /** the session base url */
    char base_url[256];
    /** the base url for resuming */
    char resume_url[256];
    /** session limits */
    struct discord_session_start_limit start_limit;
    /** active concurrent sessions */
    int concurrent;
    /** event counter to avoid reaching limit of 120 events per 60 sec */
    int event_count;
    /** @ref DiscordInternalGatewaySessionStatus */
    unsigned status;

    /** retry connection structure */
    struct {
        /** will attempt reconnecting if true */
        bool enable;
        /** current retry attempt (resets to 0 when succesful) */
        int attempt;
        /** max amount of retries before giving up */
        int limit;
    } retry;
};

/** @brief The handle for storing the Discord response payload */
struct discord_gateway_payload {
    /** current iteration JSON */
    struct {
        /** the JSON text */
        char *start;
        /** the text length */
        size_t size;
        /** jsmn tokens */
        jsmntok_t *tokens;
        /** amount of jsmn tokens */
        unsigned ntokens;
        /** jsmn-find key/value pairs */
        jsmnf_pair *pairs;
        /** amount of jsmn-find key/value pairs */
        unsigned npairs;
    } json;

    /** field 'op' */
    enum discord_gateway_opcodes opcode;
    /** field 's' */
    int seq;
    /** field 't' */
    char name[32];
    /** field 't' enumerator value */
    enum discord_gateway_events event;
    /** field 'd' */
    jsmnf_pair *data;
};

/** A generic event callback for casting */
typedef void (*discord_ev_event)(struct discord *client, const void *event);
/** An event callback for @ref DISCORD_EV_MESSAGE_CREATE */
typedef void (*discord_ev_message)(struct discord *client,
                                   const struct discord_message *event);

/** @brief The handle used for interfacing with Discord's Gateway API */
struct discord_gateway {
    /** `DISCORD_GATEWAY` logging module */
    struct logconf conf;
    /** the websockets handle that connects to Discord */
    struct websockets *ws;
    /** curl_multi handle for non-blocking transfer over websockets */
    CURLM *mhandle;

    /** timers kept for synchronization */
    struct {
        /**
         * fixed milliseconds interval between heartbeats
         * @note obtained at `HELLO`
         */
        int64_t hbeat_interval;
        /**
         * boolean that indicates if the last heartbeat was answered
         * @note used to detect zombie connections
        */
        bool hbeat_acknowledged;
        /**
         * Gateway's concept of "now"
         * @note updated at discord_gateway_perform()
         */
        u64unix_ms now;
        /**
         * last heartbeat pulse timestamp
         * @note first sent at `READY` and `RESUME`, then updated every
         *      `hbeat_interval`
         */
        u64unix_ms hbeat_last;
        /**
         * timestamp of last succesful identify request
         * @note updated at discord_gateway_send_identify()
         */
        u64unix_ms identify_last;
        /**
         * timestamp of last succesful event
         * @note resets every 60s
         */
        u64unix_ms event;
        /** timer id for heartbeat timer */
        unsigned hbeat_timer;

        /**
         * latency obtained from `HEARTBEAT` and `HEARTBEAT_ACK` response
         *      interval
         */
        int ping_ms;
        /** ping rwlock  */
        pthread_rwlock_t rwlock;
    } *timer;

    /** the identify structure for client authentication */
    struct discord_identify id;

    /** on-going session structure */
    struct discord_gateway_session *session;

    /** response-payload structure */
    struct discord_gateway_payload payload;
    /**
     * the user's callbacks for Discord events
     * @note index 0 for cache callbacks, index 1 for user callbacks
     * @todo should be cast to the original callback signature before calling,
     *      otherwise its UB
     */
    discord_ev_event cbs[2][DISCORD_EV_MAX];
    /** the event scheduler callback */
    discord_ev_scheduler scheduler;
};

/**
 * @brief Initialize a Gateway handle
 *
 * Structure used for interfacing with the Discord's Gateway API
 * @param gw the gateway handle to be initialized
 * @param conf pointer to @ref discord logging module
 * @param token the bot token
 */
void discord_gateway_init(struct discord_gateway *gw,
                          struct logconf *conf,
                          const char token[]);

/**
 * @brief Free a Gateway handle
 *
 * @param gw the handle initialized with discord_gateway_init()
 */
void discord_gateway_cleanup(struct discord_gateway *gw);

/**
 * @brief Initialize handle with the new session primitives
 *
 * @param gw the handle initialized with discord_gateway_init()
 * @CCORD_return
 */
CCORDcode discord_gateway_start(struct discord_gateway *gw);

/**
 * @brief Cleanup and reset `gw` session primitives
 *
 * @param ws the WebSockets handle created with ws_init()
 * @return `true` if session is over, `false` if session can be retried for
 *      reconnection
 */
bool discord_gateway_end(struct discord_gateway *gw);

/**
 * @brief Check and manage on-going Gateway session
 *
 * @param req the request handler
 * @CCORD_return
 */
CCORDcode discord_gateway_perform(struct discord_gateway *gw);

/**
 * @brief Gracefully shutdown a ongoing Discord connection over WebSockets
 *
 * @param gw the handle initialized with discord_gateway_init()
 */
void discord_gateway_shutdown(struct discord_gateway *gw);

/**
 * @brief Gracefully reconnect a ongoing Discord connection over WebSockets
 *
 * @param gw the handle initialized with discord_gateway_init()
 * @param resume true to attempt to resume to previous session,
 *        false restart a fresh session
 */
void discord_gateway_reconnect(struct discord_gateway *gw, bool resume);

/**
 * @brief Trigger the initial handshake with the gateway
 *
 * @param gw the handle initialized with discord_gateway_init()
 * @param event provide client identification information
 */
void discord_gateway_send_identify(struct discord_gateway *gw,
                                   struct discord_identify *event);

/**
 * @brief Replay missed events when a disconnected client resumes
 *
 * @param gw the handle initialized with discord_gateway_init()
 * @param event session resume information
 */
void discord_gateway_send_resume(struct discord_gateway *gw,
                                 struct discord_resume *event);

/**
 * @brief Maintain an active gateway connection
 *
 * @param gw the handle initialized with discord_gateway_init()
 * @param seq the last session sequence number
 */
void discord_gateway_send_heartbeat(struct discord_gateway *gw, int seq);

/**
 * @brief Request all members for a guild or a list of guilds.
 *
 * @param gw the handle initialized with discord_gateway_init()
 * @param event request guild members information
 */
void discord_gateway_send_request_guild_members(
    struct discord_gateway *gw, struct discord_request_guild_members *event);

/**
 * @brief Sent when a client wants to join, move or disconnect from a voice
 *      channel
 *
 * @param gw the handle initialized with discord_gateway_init()
 * @param event request guild members information
 */
void discord_gateway_send_update_voice_state(
    struct discord_gateway *gw, struct discord_update_voice_state *event);

/**
 * @brief Send client's presence status update payload
 *
 * @param gw the handle initialized with discord_gateway_init()
 * @param event the presence to be set
 */
void discord_gateway_send_presence_update(
    struct discord_gateway *gw, struct discord_presence_update *event);

/**
 * @brief Dispatch user callback matched to event
 *
 * @param gw the handle initialized with discord_gateway_init()
 */
void discord_gateway_dispatch(struct discord_gateway *gw);

/** @} DiscordInternalGateway */

/** @defgroup DiscordInternalRefcount Reference counter
 * @brief Handle automatic cleanup of user's data
 *  @{ */

/**
 * @brief Automatically cleanup user data
 *
 * Automatically cleanup user data that is passed around Discord event's
 *      callbacks once its reference counter reaches 0, meaning there are no
 *      more callbacks expecting the data
 */
struct discord_refcounter {
    /** `DISCORD_REFCOUNT` logging module */
    struct logconf conf;

    struct anomap *maps[16];
    pthread_mutex_t locks[16];
};

/**
 * @brief Initialize reference counter handle
 *
 * A hashtable shall be used for storage and retrieval of user data
 * @param rc the reference counter handle to be initialized
 * @param conf pointer to @ref discord logging module
 */
void discord_refcounter_init(struct discord_refcounter *rc,
                             struct logconf *conf);

/**
 * @brief Add a new internal reference to the reference counter
 *
 * @param rc the handle initialized with discord_refcounter_init()
 * @param data the data address to be referenced
 * @param cleanup function for cleaning `data` resources once its
 *      no longer referenced
 * @param should_free whether `data` cleanup should be followed by a free()
 */
#define discord_refcounter_add_internal(refcounter, data, cleanup,            \
                                        should_free)                          \
    __discord_refcounter_add_internal(refcounter, __func__, data, cleanup,    \
                                      should_free)
void __discord_refcounter_add_internal(struct discord_refcounter *rc,
                                       const char *name,
                                       void *data,
                                       void (*cleanup)(void *data),
                                       bool should_free);

/**
 * @brief Add a new client reference to the reference counter
 *
 * @param rc the handle initialized with discord_refcounter_init()
 * @param data the data address to be referenced
 * @param cleanup function for cleaning `data` resources once its
 *      no longer referenced
 * @param should_free whether `data` cleanup should be followed by a free()
 */
#define discord_refcounter_add_client(refcounter, data, cleanup, should_free) \
    __discord_refcounter_add_client(refcounter, __func__, data, cleanup,      \
                                    should_free)
void __discord_refcounter_add_client(struct discord_refcounter *rc,
                                     const char *name,
                                     void *data,
                                     void (*cleanup)(struct discord *client,
                                                     void *data),
                                     bool should_free);

/**
 * @brief Cleanup refcounter and all user data currently held
 *
 * @param rc the handle initialized with discord_refcounter_init()
 */
void discord_refcounter_cleanup(struct discord_refcounter *rc);

/**
 * @brief Claim ownership of `data`
 * @see discord_refcounter_unclaim()
 *
 * After ownership is claimed `data` will no longer be cleaned automatically,
 *      instead shall be cleaned only when the same amount of
 *      discord_refcounter_unclaim() have been called
 * @param rc the handle initialized with discord_refcounter_init()
 * @param data the data to have its ownership claimed
 * @retval CCORD_OK counter for `data` has been incremented
 * @retval CCORD_RESOURCE_UNAVAILABLE couldn't find a match to `data`
 */
CCORDcode discord_refcounter_claim(struct discord_refcounter *rc,
                                   const void *data);

/**
 * @brief Unclaim ownership of `data`
 * @see discord_refcounter_claim()
 *
 * This will make the resource eligible for cleanup, so this should only be
 *      called when you no longer plan to use it
 * @param rc the handle initialized with discord_refcounter_init()
 * @param data the data to have its ownership unclaimed
 * @retval CCORD_OK counter for `data` has been decremented
 * @retval CCORD_RESOURCE_UNAVAILABLE couldn't find a match to `data`
 * @retval CCORD_RESOURCE_OWNERSHIP `data` has never been discord_claim() 'd
 */
CCORDcode discord_refcounter_unclaim(struct discord_refcounter *rc,
                                     void *data);

/**
 * @brief Increment the reference counter for `ret->data`
 * @see discord_refcounter_decr()
 *
 * @param rc the handle initialized with discord_refcounter_init()
 * @param data the data to have its reference counter incremented
 * @retval CCORD_OK counter for `data` has been incremented
 * @retval CCORD_RESOURCE_UNAVAILABLE couldn't find a match to `data`
 */
CCORDcode discord_refcounter_incr(struct discord_refcounter *rc, void *data);

/**
 * @brief Decrement the reference counter for `data`
 * @see discord_refcounter_incr()
 *
 * If the count reaches zero then `data` shall be cleanup up with its
 *      user-defined cleanup function
 * @param rc the handle initialized with discord_refcounter_init()
 * @param data the data to have its reference counter decremented
 * @retval CCORD_OK counter for `data` has been decremented
 * @retval CCORD_RESOURCE_UNAVAILABLE couldn't find a match to `data`
 * @retval CCORD_RESOURCE_OWNERSHIP caught attempt to cleanup a claimed
 * resource
 */
CCORDcode discord_refcounter_decr(struct discord_refcounter *rc, void *data);

/** @} DiscordInternalRefcount */

/** @defgroup DiscordInternalMessageCommands Message Commands API
 * @brief The Message Commands API for registering and parsing user commands
 *  @{ */

/**
 * @brief The handle for storing user's message commands
 * @see discord_set_on_command()
 */
struct discord_message_commands {
    /** `DISCORD_MESSAGE_COMMANDS` logging module */
    struct logconf conf;
    /** the prefix expected for every command */
    struct ccord_szbuf prefix;
    /** fallback message command @see discord_set_on_command() */
    discord_ev_message fallback;
    /** amount of message commands created */
    int length;
    /** message commands cap before increase */
    int capacity;
    /**
     * message command entries
     * @note datatype declared at discord-messagecommands.c
     */
    struct _discord_message_commands_entry *entries;
};

/**
 * @brief Initialize a Message Commands handle
 *
 * @param cmds the message commands handle to be initialized
 * @param conf pointer to @ref discord logging module
 */
void discord_message_commands_init(struct discord_message_commands *cmds,
                                   struct logconf *conf);

/**
 * @brief Free a Message Commands handle
 *
 * @param cmds the handle initialized with discord_message_commands_init()
 */
void discord_message_commands_cleanup(struct discord_message_commands *cmds);

/**
 * @brief Search for a callback matching the command
 *
 * @param cmds the handle initialized with discord_message_commands_init()
 * @param command the command to be searched for
 * @param length the command length
 * @return the callback match, `NULL` in case there wasn't a match
 */
discord_ev_message discord_message_commands_find(
    struct discord_message_commands *cmds,
    const char command[],
    size_t length);

/**
 * @brief Add a new command/callback pair, or update an existing command
 *
 * @param cmds the handle initialized with discord_message_commands_init()
 * @param command the message command to be matched with callback
 * @param length the command length
 * @param callback the callback to be triggered when the command is sent
 */
void discord_message_commands_append(struct discord_message_commands *cmds,
                                     const char command[],
                                     size_t length,
                                     discord_ev_message callback);

/**
 * @brief Set a mandatory prefix before commands
 * @see discord_set_on_command()
 *
 * Example: If @a 'help' is a command and @a '!' prefix is set, the command
 *       will only be validated if @a '!help' is sent
 * @param cmds the handle initialized with discord_message_commands_init()
 * @param prefix the mandatory command prefix
 * @param length the prefix length
 */
void discord_message_commands_set_prefix(struct discord_message_commands *cmds,
                                         const char prefix[],
                                         size_t length);

/**
 * @brief Read the current @ref DISCORD_EV_MESSAGE_CREATE payload and attempt
 *      to perform its matching callback
 *
 * @param cmds the handle initialized with discord_message_commands_init()
 * @param payload the event payload to read from
 *      (assumes its from `MESSAGE_CREATE`)
 * @return `true` if the callback has been performed
 */
bool discord_message_commands_try_perform(
    struct discord_message_commands *cmds,
    struct discord_gateway_payload *payload);

/** @} DiscordInternalMessageCommands */

/** @defgroup DiscordInternalCache Cache API
 * @brief The Cache API for storage and retrieval of Discord data
 *  @{ */

/**
 * @brief The Discord Cache control handler
 */
struct discord_cache {
    struct _discord_cache_data *data;
    void (*cleanup)(struct discord *client);

    /** gateway should call this when a shard has lost connection */
    void (*on_shard_disconnected)(struct discord *client,
                                  const struct discord_identify *ident,
                                  bool resumable);
    /** gateway should call this when a shard has resumed */
    void (*on_shard_resumed)(struct discord *client,
                             const struct discord_identify *ident);
    /** gateway should call this when a shard has reconnected */
    void (*on_shard_reconnected)(struct discord *client,
                                 const struct discord_identify *ident);
};

/** @} DiscordInternalCache */

/**
 * @brief The Discord client handler
 *
 * Used to access/perform public functions from discord.h
 * @see discord_init(), discord_config_init(), discord_cleanup()
 */
struct discord {
    /** `DISCORD` logging module */
    struct logconf conf;
    /** whether this is the original client or a clone */
    bool is_original;
    /** the bot token */
    char *token;
    /** the io poller for listening to file descriptors */
    struct io_poller *io_poller;

    /** the user's message commands @see discord_set_on_command() */
    struct discord_message_commands commands;
    /** user's data reference counter for automatic cleanup */
    struct discord_refcounter refcounter;

    /** the handle for interfacing with Discord's REST API */
    struct discord_rest rest;
    /** the handle for interfacing with Discord's Gateway API */
    struct discord_gateway gw;
    /** the client's user structure */
    struct discord_user self;
    /** the handle for registering and retrieving Discord data */
    struct discord_cache cache;

    /** fd that gets triggered when ccord_shutdown_async is called */
    int shutdown_fd;

    struct {
        struct discord_timers internal;
        struct discord_timers user;
    } timers;

    /** wakeup timer handle */
    struct {
        /** callback to be triggered on timer's timeout */
        void (*cb)(struct discord *client);
        /** the id of the wake timer */
        unsigned id;
    } wakeup_timer;

    /** triggers when idle */
    void (*on_idle)(struct discord *client);
    /** triggers once per loop cycle */
    void (*on_cycle)(struct discord *client);

    /** user arbitrary data @see discord_set_data() */
    void *data;

    /** keep tab of amount of worker threads being used by client */
    struct {
        /** amount of worker-threads currently being used by client */
        int count;
        /** synchronize `count` between workers */
        pthread_mutex_t lock;
        /** notify of `count` decrement */
        pthread_cond_t cond;
    } *workers;

#ifdef CCORD_VOICE
    struct discord_voice *vcs;
    struct discord_voice_evcallbacks voice_cbs;
#endif /* CCORD_VOICE */
};

/** @} DiscordInternal */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* DISCORD_INTERNAL_H */
