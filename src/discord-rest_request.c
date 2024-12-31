#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "discord.h"
#include "discord-internal.h"

static struct discord_request *
_discord_request_init(void)
{
    return calloc(1, sizeof(struct discord_request));
}

static void
_discord_request_cleanup(struct discord_request *req)
{
    discord_attachments_cleanup(&req->attachments);
    if (req->body.start) free(req->body.start);
    if (req->reason) free(req->reason);
    free(req);
}

static void
_discord_on_curl_setopt(struct ua_conn *conn, void *p_token)
{
    char auth[128];
    int len = snprintf(auth, sizeof(auth), "Bot %s", (char *)p_token);
    ASSERT_NOT_OOB(len, sizeof(auth));

    ua_conn_add_header(conn, "Authorization", auth);
#ifdef CCORD_DEBUG_HTTP
    curl_easy_setopt(ua_conn_get_easy_handle(conn), CURLOPT_VERBOSE, 1L);
#endif
}

void
discord_requestor_init(struct discord_requestor *rqtor,
                       struct logconf *conf,
                       const char token[])
{
    logconf_branch(&rqtor->conf, conf, "DISCORD_REQUEST");

    rqtor->ua = ua_init(&(struct ua_attr){ .conf = conf });
    ua_set_url(rqtor->ua, DISCORD_API_BASE_URL);
    ua_set_opt(rqtor->ua, (char *)token, &_discord_on_curl_setopt);

    /* queues are malloc'd to guarantee a client cloned by
     * discord_clone() will share the same queue with the original */
    rqtor->queues = malloc(sizeof *rqtor->queues);
    QUEUE_INIT(&rqtor->queues->recycling);
    QUEUE_INIT(&rqtor->queues->pending);
    QUEUE_INIT(&rqtor->queues->finished);

    rqtor->qlocks = malloc(sizeof *rqtor->qlocks);
    ASSERT_S(!pthread_mutex_init(&rqtor->qlocks->recycling, NULL),
             "Couldn't initialize requestor's recycling queue mutex");
    ASSERT_S(!pthread_mutex_init(&rqtor->qlocks->pending, NULL),
             "Couldn't initialize requestor's pending queue mutex");
    ASSERT_S(!pthread_mutex_init(&rqtor->qlocks->finished, NULL),
             "Couldn't initialize requestor's finished queue mutex");

    rqtor->mhandle = curl_multi_init();
    rqtor->retry_limit = 3; /* FIXME: shouldn't be a hard limit */

    discord_ratelimiter_init(&rqtor->ratelimiter, &rqtor->conf);
}

void
discord_requestor_cleanup(struct discord_requestor *rqtor)
{
    struct discord_rest *rest =
        CONTAINEROF(rqtor, struct discord_rest, requestor);
    QUEUE *const req_queues[] = { &rqtor->queues->recycling,
                                  &rqtor->queues->pending,
                                  &rqtor->queues->finished };

    /* cleanup ratelimiting handle */
    discord_ratelimiter_cleanup(&rqtor->ratelimiter);

    /* cleanup queues */
    for (size_t i = 0; i < sizeof(req_queues) / sizeof *req_queues; ++i) {
        QUEUE(struct discord_request) queue, *qelem;
        struct discord_request *req;

        QUEUE_MOVE(req_queues[i], &queue);
        while (!QUEUE_EMPTY(&queue)) {
            qelem = QUEUE_HEAD(&queue);
            QUEUE_REMOVE(qelem);

            req = QUEUE_DATA(qelem, struct discord_request, entry);
            _discord_request_cleanup(req);
        }
    }
    free(rqtor->queues);

    /* cleanup queue locks */
    pthread_mutex_destroy(&rqtor->qlocks->recycling);
    pthread_mutex_destroy(&rqtor->qlocks->pending);
    pthread_mutex_destroy(&rqtor->qlocks->finished);
    free(rqtor->qlocks);

    /* cleanup curl's multi handle */
    io_poller_curlm_del(rest->io_poller, rqtor->mhandle);
    curl_multi_cleanup(rqtor->mhandle);
    /* cleanup User-Agent handle */
    ua_cleanup(rqtor->ua);
}

static void
_discord_request_to_multipart(curl_mime *mime, void *p_req)
{
    struct discord_request *req = p_req;
    curl_mimepart *part;
    char name[64];

    /* json part */
    if (req->body.start && req->body.size) {
        part = curl_mime_addpart(mime);
        curl_mime_data(part, req->body.start, req->body.size);
        curl_mime_type(part, "application/json");
        curl_mime_name(part, "payload_json");
    }

    /* attachment part */
    for (int i = 0; i < req->attachments.size; ++i) {
        int len = snprintf(name, sizeof(name), "files[%" PRIu64 "]",
                           req->attachments.array[i].id);
        ASSERT_NOT_OOB(len, sizeof(name));

        if (req->attachments.array[i].content) {
            part = curl_mime_addpart(mime);
            curl_mime_data(part, req->attachments.array[i].content,
                           req->attachments.array[i].size
                               ? req->attachments.array[i].size
                               : CURL_ZERO_TERMINATED);
            curl_mime_filename(part, !req->attachments.array[i].filename
                                         ? "a.out"
                                         : req->attachments.array[i].filename);
            curl_mime_type(part, !req->attachments.array[i].content_type
                                     ? "application/octet-stream"
                                     : req->attachments.array[i].content_type);
            curl_mime_name(part, name);
        }
        else if (req->attachments.array[i].filename) {
            CURLcode ecode;

            /* fetch local file by the filename */
            part = curl_mime_addpart(mime);
            ecode =
                curl_mime_filedata(part, req->attachments.array[i].filename);
            if (ecode != CURLE_OK) {
                char errbuf[256];
                snprintf(errbuf, sizeof(errbuf), "%s (file: %s)",
                         curl_easy_strerror(ecode),
                         req->attachments.array[i].filename);
                perror(errbuf);
                continue;
            }
            curl_mime_type(part, !req->attachments.array[i].content_type
                                     ? "application/octet-stream"
                                     : req->attachments.array[i].content_type);
            curl_mime_name(part, name);
        }
    }
}

static bool
_discord_request_info_extract(struct discord_requestor *rqtor,
                              struct discord_request *req,
                              struct ua_info *info)
{
    ua_info_extract(req->conn, info);

    if (info->code != CCORD_HTTP_CODE) { /* CCORD_OK or internal error */
        req->code = info->code;
        return false;
    }

    switch (info->httpcode) {
    case HTTP_FORBIDDEN:
    case HTTP_NOT_FOUND:
    case HTTP_BAD_REQUEST:
        req->code = CCORD_DISCORD_JSON_CODE;
        return false;
    case HTTP_UNAUTHORIZED:
        logconf_fatal(
            &rqtor->conf,
            "UNAUTHORIZED: Please provide a valid authentication token");
        req->code = CCORD_DISCORD_BAD_AUTH;
        return false;
    case HTTP_METHOD_NOT_ALLOWED:
        logconf_fatal(&rqtor->conf,
                      "METHOD_NOT_ALLOWED: The server couldn't recognize the "
                      "received HTTP method");

        req->code = info->code;
        return false;
    case HTTP_TOO_MANY_REQUESTS: {
        struct ua_szbuf_readonly body = ua_info_get_body(info);
        struct jsmnftok message = { 0 };
        u64unix_ms retry_after_ms = 1000;
        bool is_global = false;
        jsmn_parser parser;
        jsmntok_t tokens[16];

        jsmn_init(&parser);
        if (0 < jsmn_parse(&parser, body.start, body.size, tokens,
                           sizeof(tokens) / sizeof *tokens))
        {
            jsmnf_loader loader;
            jsmnf_pair pairs[16];

            jsmnf_init(&loader);
            if (0 < jsmnf_load(&loader, body.start, tokens, parser.toknext,
                               pairs, sizeof(pairs) / sizeof *pairs))
            {
                jsmnf_pair *f;

                if ((f = jsmnf_find(pairs, body.start, "global", 6)))
                    is_global = ('t' == body.start[f->v.pos]);
                if ((f = jsmnf_find(pairs, body.start, "message", 7)))
                    message = f->v;
                if ((f = jsmnf_find(pairs, body.start, "retry_after", 11))) {
                    double retry_after = strtod(body.start + f->v.pos, NULL);
                    if (retry_after > 0)
                        retry_after_ms = (u64unix_ms)(1000 * retry_after);
                }
            }
        }

        logconf_warn(&rqtor->conf,
                     "429 %sRATELIMITING (wait: %" PRIu64 " ms) : %.*s",
                     is_global ? "GLOBAL " : "", retry_after_ms, message.len,
                     body.start + message.pos);

        if (is_global)
            discord_ratelimiter_set_global_timeout(&rqtor->ratelimiter, req->b,
                                                   retry_after_ms);
        else
            discord_bucket_set_timeout(req->b, retry_after_ms);

        req->code = info->code;
        return true;
    }
    default:
        req->code = info->code;
        return (info->httpcode >= 500); /* retry if Server Error */
    }
}

void
discord_request_cancel(struct discord_requestor *rqtor,
                       struct discord_request *req)
{
    struct discord_refcounter *rc = &CLIENT(rqtor, rest.requestor)->refcounter;

    if (NOT_EMPTY_STR(req->reason)) {
        ua_conn_remove_header(req->conn, "X-Audit-Log-Reason");
        free(req->reason);
        req->reason = NULL;
    }
    if (req->conn) {
        ua_conn_stop(req->conn);
    }
    if (req->dispatch.keep) {
        discord_refcounter_decr(rc, (void *)req->dispatch.keep);
    }
    if (req->dispatch.data) {
        discord_refcounter_decr(rc, req->dispatch.data);
    }

    req->body.size = 0;
    req->method = 0;
    *req->endpoint = '\0';
    *req->key = '\0';
    req->conn = NULL;
    req->retry_attempt = 0;
    discord_attachments_cleanup(&req->attachments);
    memset(req, 0, sizeof(struct discord_attributes));

    QUEUE_REMOVE(&req->entry);
    pthread_mutex_lock(&rqtor->qlocks->recycling);
    QUEUE_INSERT_TAIL(&rqtor->queues->recycling, &req->entry);
    pthread_mutex_unlock(&rqtor->qlocks->recycling);
}

static CCORDcode
_discord_request_dispatch_response(struct discord_requestor *rqtor,
                                   struct discord_request *req)
{
    struct discord *client = CLIENT(rqtor, rest.requestor);
    struct discord_response resp = { .data = req->dispatch.data,
                                     .keep = req->dispatch.keep,
                                     .code = req->code };

    if (req->code != CCORD_OK) {
        if (req->dispatch.fail) req->dispatch.fail(client, &resp);
    }
    else if (req->dispatch.done.typed) {
        if (!req->dispatch.has_type) {
            req->dispatch.done.typeless(client, &resp);
        }
        else {
            req->dispatch.done.typed(client, &resp, req->response.data);
            discord_refcounter_decr(&client->refcounter, req->response.data);
        }
    }
    /* enqueue request for recycle */
    discord_request_cancel(rqtor, req);

    return resp.code;
}

void
discord_requestor_dispatch_responses(struct discord_requestor *rqtor)
{
    if (0 == pthread_mutex_trylock(&rqtor->qlocks->finished)) {
        QUEUE(struct discord_request) queue;
        QUEUE_MOVE(&rqtor->queues->finished, &queue);
        pthread_mutex_unlock(&rqtor->qlocks->finished);

        if (!QUEUE_EMPTY(&queue)) {
            struct discord_rest *rest =
                CONTAINEROF(rqtor, struct discord_rest, requestor);
            QUEUE(struct discord_request) * qelem;
            struct discord_request *req;

            do {
                qelem = QUEUE_HEAD(&queue);
                req = QUEUE_DATA(qelem, struct discord_request, entry);
                _discord_request_dispatch_response(rqtor, req);
            } while (!QUEUE_EMPTY(&queue));
            io_poller_wakeup(rest->io_poller);
        }
    }
}

/**
 * @brief If request can be retried then it will be moved back to its
 *      bucket's queue
 * @note this **MUST** be called only after discord_request_info_extract()
 *
 * @param rqtor the requestor handle initialized with discord_requestor_init()
 * @param req the request to be checked for retry
 * @return `true` if request has been enqueued for retry
 */
static bool
_discord_request_retry(struct discord_requestor *rqtor,
                       struct discord_request *req)
{
    if (req->retry_attempt++ >= rqtor->retry_limit) return false;

    ua_conn_reset(req->conn);
    discord_bucket_insert(&rqtor->ratelimiter, req->b, req, true);

    return true;
}

CCORDcode
discord_requestor_info_read(struct discord_requestor *rqtor)
{
    int alive = 0;

    if (CURLM_OK != curl_multi_socket_all(rqtor->mhandle, &alive))
        return CCORD_CURLM_INTERNAL;

    /* ask for any messages/informationals from the individual transfers */
    while (1) {
        int msgq = 0;
        struct CURLMsg *msg = curl_multi_info_read(rqtor->mhandle, &msgq);

        if (!msg) break;

        if (CURLMSG_DONE == msg->msg) {
            const CURLcode ecode = msg->data.result;
            struct discord_request *req;
            bool retry = false;

            curl_easy_getinfo(msg->easy_handle, CURLINFO_PRIVATE, &req);
            curl_multi_remove_handle(rqtor->mhandle, msg->easy_handle);

            switch (ecode) {
            case CURLE_OK: {
                struct ua_szbuf_readonly body;
                struct ua_info info;

                retry = _discord_request_info_extract(rqtor, req, &info);
                body = ua_info_get_body(&info);

                if (info.code != CCORD_OK) {
                    logconf_error(&rqtor->conf, "%.*s", (int)body.size,
                                  body.start);
                }
                else if (req->dispatch.has_type
                         && req->dispatch.sync != DISCORD_SYNC_FLAG)
                {
                    if (req->dispatch.sync) {
                        req->response.data = req->dispatch.sync;
                    }
                    else {
                        req->response.data = calloc(1, req->response.size);
                        discord_refcounter_add_internal(
                            &CLIENT(rqtor, rest.requestor)->refcounter,
                            req->response.data, req->response.cleanup, true);
                    }

                    /* initialize ret */
                    if (req->response.init)
                        req->response.init(req->response.data);
                    /* populate ret */
                    if (req->response.from_json)
                        req->response.from_json(body.start, body.size,
                                                req->response.data);
                }

                /** FIXME: bucket should be recycled if it was matched with an
                 *      invalid endpoint */
                discord_ratelimiter_build(&rqtor->ratelimiter, req->b,
                                          req->key, &info);

                ua_info_cleanup(&info);
            } break;
            default:
                logconf_warn(&rqtor->conf, "%s (CURLE code: %d)",
                             curl_easy_strerror(ecode), ecode);

                retry = (ecode == CURLE_READ_ERROR);
                req->code = CCORD_CURLE_INTERNAL;
                break;
            }

            if (!retry || !_discord_request_retry(rqtor, req)) {
                discord_bucket_request_unselect(&rqtor->ratelimiter, req->b,
                                                req);

                if (req->dispatch.sync) {
                    pthread_mutex_lock(&rqtor->qlocks->pending);
                    pthread_cond_signal(req->cond);
                    pthread_mutex_unlock(&rqtor->qlocks->pending);
                }
                else {
                    pthread_mutex_lock(&rqtor->qlocks->finished);
                    QUEUE_INSERT_TAIL(&rqtor->queues->finished, &req->entry);
                    pthread_mutex_unlock(&rqtor->qlocks->finished);
                }
            }
        }
    }

    return CCORD_OK;
}

static void
_discord_request_send(void *p_rqtor, struct discord_request *req)
{
    static struct ua_szbuf_readonly hide_headers[] = {
        { "Authorization", sizeof("Authorization") - 1 }
    };

    struct discord_requestor *rqtor = p_rqtor;
    CURL *ehandle;

    req->conn = ua_conn_start(rqtor->ua);
    ehandle = ua_conn_get_easy_handle(req->conn);

    if (NOT_EMPTY_STR(req->reason))
        ua_conn_add_header(req->conn, "X-Audit-Log-Reason", req->reason);

    if (HTTP_MIMEPOST == req->method) {
        ua_conn_add_header(req->conn, "Content-Type", "multipart/form-data");
        ua_conn_set_mime(req->conn, req, &_discord_request_to_multipart);
    }
    else if (req->body.size)
        ua_conn_add_header(req->conn, "Content-Type", "application/json");
    else
        ua_conn_remove_header(req->conn, "Content-Type");

    ua_conn_setup(req->conn, &(struct ua_conn_attr){
                                 .method = req->method,
                                 .body = req->body.start,
                                 .body_size = req->body.size,
                                 .endpoint = req->endpoint,
                                 .base_url = NULL,
                                 .log_filter = {
                                    .headers = hide_headers,
                                    .length = sizeof(hide_headers) / sizeof *hide_headers,
                                 },
                             });

    /* link 'req' to 'ehandle' for easy retrieval */
    curl_easy_setopt(ehandle, CURLOPT_PRIVATE, req);

    /* initiate libcurl transfer */
    curl_multi_add_handle(rqtor->mhandle, ehandle);
}

CCORDcode
discord_requestor_start_pending(struct discord_requestor *rqtor)
{
    QUEUE(struct discord_request) queue, *qelem;
    struct discord_request *req;
    struct discord_bucket *b;

    pthread_mutex_lock(&rqtor->qlocks->pending);
    QUEUE_MOVE(&rqtor->queues->pending, &queue);
    pthread_mutex_unlock(&rqtor->qlocks->pending);

    /* match pending requests to their buckets */
    while (!QUEUE_EMPTY(&queue)) {
        qelem = QUEUE_HEAD(&queue);
        QUEUE_REMOVE(qelem);

        req = QUEUE_DATA(qelem, struct discord_request, entry);
        b = discord_bucket_get(&rqtor->ratelimiter, req->key);
        discord_bucket_insert(&rqtor->ratelimiter, b, req,
                              req->dispatch.high_priority);
    }

    discord_bucket_request_selector(&rqtor->ratelimiter, rqtor,
                                    &_discord_request_send);

    /* FIXME: redundant return value (constant) */
    return CCORD_OK;
}

/* Only fields required at _discord_request_to_multipart() are duplicated */
static void
_discord_attachments_dup(struct discord_attachments *dest,
                         const struct discord_attachments *src)
{
    int i;

    __carray_init(dest, (size_t)src->size, struct discord_attachment, , );
    for (i = 0; i < src->size; ++i) {
        carray_insert(dest, i, src->array[i]);
        if (src->array[i].content) {
            dest->array[i].size = src->array[i].size
                                      ? src->array[i].size
                                      : strlen(src->array[i].content) + 1;
            dest->array[i].content = malloc(dest->array[i].size);
            memcpy(dest->array[i].content, src->array[i].content,
                   dest->array[i].size);
        }
        if (src->array[i].filename)
            cog_strndup(src->array[i].filename, strlen(src->array[i].filename),
                        &dest->array[i].filename);
        if (src->array[i].content_type)
            cog_strndup(src->array[i].content_type,
                        strlen(src->array[i].content_type),
                        &dest->array[i].content_type);
    }
    dest->size = i;
}

static void
_discord_request_attributes_copy(struct discord_request *dest,
                                 const struct discord_attributes *src)
{
    dest->dispatch = src->dispatch;
    dest->response = src->response;
    dest->attachments = src->attachments;
    if (src->reason) { /* request reason if included */
        if (!dest->reason) dest->reason = calloc(DISCORD_MAX_REASON_LEN, 1);
        snprintf(dest->reason, DISCORD_MAX_REASON_LEN, "%s", src->reason);
    }
    if (src->attachments.size)
        _discord_attachments_dup(&dest->attachments, &src->attachments);
}

static struct discord_request *
_discord_request_get(struct discord_requestor *rqtor)
{
    struct discord_request *req;

    pthread_mutex_lock(&rqtor->qlocks->recycling);
    if (QUEUE_EMPTY(&rqtor->queues->recycling)) { /* new request struct */
        req = _discord_request_init();
    }
    else { /* fetch a request struct from queues->recycling */
        QUEUE(struct discord_request) *qelem =
            QUEUE_HEAD(&rqtor->queues->recycling);

        QUEUE_REMOVE(qelem);
        req = QUEUE_DATA(qelem, struct discord_request, entry);
    }
    pthread_mutex_unlock(&rqtor->qlocks->recycling);

    QUEUE_INIT(&req->entry);

    return req;
}

CCORDcode
discord_request_begin(struct discord_requestor *rqtor,
                      struct discord_attributes *attr,
                      struct ccord_szbuf *body,
                      enum http_method method,
                      char endpoint[DISCORD_ENDPT_LEN],
                      char key[DISCORD_ROUTE_LEN])
{
    struct discord_rest *rest =
        CONTAINEROF(rqtor, struct discord_rest, requestor);
    struct discord *client = CLIENT(rest, rest);

    struct discord_request *req = _discord_request_get(rqtor);
    CCORDcode code;

    req->method = method;
    if (body) {
        if (body->size > req->body.realsize) { /* buffer needs a resize */
            void *tmp = realloc(req->body.start, body->size);
            ASSERT_S(tmp != NULL, "Out of memory");

            req->body.start = tmp;
            req->body.realsize = body->size;
        }
        memcpy(req->body.start, body->start, body->size);
        req->body.size = body->size;
    }
    memcpy(req->endpoint, endpoint, sizeof(req->endpoint));
    memcpy(req->key, key, sizeof(req->key));

    _discord_request_attributes_copy(req, attr);

    if (req->dispatch.keep) {
        code = discord_refcounter_incr(&client->refcounter,
                                       (void *)req->dispatch.keep);

        ASSERT_S(code == CCORD_OK, "'.keep' data must be a Concord resource");
    }
    if (req->dispatch.data
        && CCORD_RESOURCE_UNAVAILABLE
               == discord_refcounter_incr(&client->refcounter,
                                          req->dispatch.data))
    {
        discord_refcounter_add_client(&client->refcounter, req->dispatch.data,
                                      req->dispatch.cleanup, false);
    }

    pthread_mutex_lock(&rqtor->qlocks->pending);
    QUEUE_INSERT_TAIL(&rqtor->queues->pending, &req->entry);
    io_poller_wakeup(rest->io_poller);
    if (!req->dispatch.sync) {
        pthread_mutex_unlock(&rqtor->qlocks->pending);
        code = CCORD_PENDING;
    }
    else { /* wait for request's completion if sync mode is active */
        pthread_cond_t temp_cond = PTHREAD_COND_INITIALIZER;
        req->cond = &temp_cond;
        pthread_cond_wait(req->cond, &rqtor->qlocks->pending);
        req->cond = NULL;
        pthread_mutex_unlock(&rqtor->qlocks->pending);
        code = _discord_request_dispatch_response(rqtor, req);
    }
    return code;
}
