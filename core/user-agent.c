#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdarg.h>
#include <ctype.h> /* isspace() */
#include <string.h>
#include <strings.h>
#include <pthread.h>

#include "user-agent.h"
#include "cog-utils.h"
#include "queue.h"

#define CURLE_LOG(conn, ecode)                                                \
    logconf_fatal(&conn->ua->conf, "(CURLE code: %d) %s", ecode,              \
                  !*conn->errbuf ? curl_easy_strerror(ecode) : conn->errbuf)

/** @brief Generic sized buffer */
struct _ua_szbuf {
    /** the buffer's start */
    char *start;
    /** the buffer's size in bytes */
    size_t size;
};

struct user_agent {
    /**
     * queue of connection nodes for easy reuse
     * @note conns are wrappers around basic CURL functionalities,
     *        each active conn is responsible for a HTTP request
     */
    struct ua_conn_queue *connq;
    /** the base_url for every conn */
    struct _ua_szbuf base_url;
    /** the user agent logging module */
    struct logconf conf;

    struct {
        /** user arbitrary data for callback */
        void *data;
        /** user callback for libcurl's easy setup */
        void (*callback)(struct ua_conn *conn, void *data);
    } setopt;
};

struct ua_conn_queue {
    /** idle connections */
    QUEUE(struct ua_conn) idle;
    /* busy connections */
    QUEUE(struct ua_conn) busy;
    /** total amount of created connection handles  */
    int total;
    /** lock for blocking queue operations */
    pthread_mutex_t lock;
};

struct ua_conn {
    /** ptr to user_agent it belongs to */
    struct user_agent *ua;
    /** the libcurl's easy handle used to perform requests */
    CURL *ehandle;
    /** informational handle on how the request went */
    struct ua_info info;

    /** request URL */
    struct _ua_szbuf url;
    /** the conn request header */
    struct curl_slist *header;

    struct {
        /** user arbitrary data for callback */
        void *data;
        /** libcurl's data structure for multipart creation */
        curl_mime *mime;
        /** user callback for multipart creation */
        void (*callback)(curl_mime *mime, void *data);
    } multipart;

    /**
     * capture curl error messages
     * @note should only be accessed after a error code returns
     * @see https://curl.se/libcurl/c/CURLOPT_ERRORBUFFER.html
     */
    char errbuf[CURL_ERROR_SIZE];

    /** connection handle queue entry */
    QUEUE entry;
};

const char *
http_code_print(int httpcode)
{
    switch (httpcode) {
    case HTTP_OK:
        return "OK";
    case HTTP_CREATED:
        return "CREATED";
    case HTTP_NO_CONTENT:
        return "NO_CONTENT";
    case HTTP_NOT_MODIFIED:
        return "NOT_MODIFIED";
    case HTTP_BAD_REQUEST:
        return "BAD_REQUEST";
    case HTTP_UNAUTHORIZED:
        return "UNAUTHORIZED";
    case HTTP_FORBIDDEN:
        return "FORBIDDEN";
    case HTTP_NOT_FOUND:
        return "NOT_FOUND";
    case HTTP_METHOD_NOT_ALLOWED:
        return "METHOD_NOT_ALLOWED";
    case HTTP_UNPROCESSABLE_ENTITY:
        return "UNPROCESSABLE_ENTITY";
    case HTTP_TOO_MANY_REQUESTS:
        return "TOO_MANY_REQUESTS";
    case HTTP_GATEWAY_UNAVAILABLE:
        return "GATEWAY_UNAVAILABLE";
    default:
        if (httpcode >= 500) return "5xx_SERVER_ERROR";
        if (httpcode >= 400) return "4xx_CLIENT_ERROR";
        if (httpcode >= 300) return "3xx_REDIRECTING";
        if (httpcode >= 200) return "2xx_SUCCESS";
        if (httpcode >= 100) return "1xx_INFO";
        return "UNUSUAL_HTTP_CODE";
    }
}

const char *
http_reason_print(int httpcode)
{
    switch (httpcode) {
    case HTTP_OK:
        return "The request was completed succesfully.";
    case HTTP_CREATED:
        return "The entity was created succesfully.";
    case HTTP_NO_CONTENT:
        return "The request completed succesfully but returned no content.";
    case HTTP_NOT_MODIFIED:
        return "The entity was not modified (no action was taken).";
    case HTTP_BAD_REQUEST:
        return "The request was improperly formatted, or the server couldn't "
               "understand it.";
    case HTTP_UNAUTHORIZED:
        return "The Authorization header was missing or invalid.";
    case HTTP_FORBIDDEN:
        return "The Authorization token you passed did not have permission to "
               "the "
               "resource.";
    case HTTP_NOT_FOUND:
        return "The resource at the location specified doesn't exist.";
    case HTTP_METHOD_NOT_ALLOWED:
        return "The HTTP method used is not valid for the location specified.";
    case HTTP_TOO_MANY_REQUESTS:
        return "You got ratelimited.";
    case HTTP_GATEWAY_UNAVAILABLE:
        return "There was not a gateway available to process your request. "
               "Wait a "
               "bit and retry.";
    default:
        if (httpcode >= 500)
            return "The server had an error processing your request.";
        if (httpcode >= 400)
            return "Client side error, request couldn't be processed.";
        if (httpcode >= 300)
            return "Client requires taking additional action to complete the "
                   "request";
        if (httpcode >= 200)
            return "The action request was received, understood and accepted.";
        if (httpcode >= 100)
            return "The request was received and understood. The client must "
                   "wait "
                   "for a final response.";
        if (!httpcode) return "Curl couldn't fetch a HTTP response.";
        return "Unusual HTTP method.";
    }
}

const char *
http_method_print(enum http_method method)
{
    switch (method) {
    case HTTP_DELETE:
        return "DELETE";
    case HTTP_GET:
        return "GET";
    case HTTP_POST:
        return "POST";
    case HTTP_MIMEPOST:
        return "MIMEPOST";
    case HTTP_PATCH:
        return "PATCH";
    case HTTP_PUT:
        return "PUT";
    case HTTP_INVALID:
    default:
        return "INVALID_HTTP_METHOD";
    }
}

enum http_method
http_method_eval(char method[])
{
    if (0 == strcmp(method, "DELETE")) return HTTP_DELETE;
    if (0 == strcmp(method, "GET")) return HTTP_GET;
    if (0 == strcmp(method, "POST")) return HTTP_POST;
    if (0 == strcmp(method, "MIMEPOST")) return HTTP_MIMEPOST;
    if (0 == strcmp(method, "PATCH")) return HTTP_PATCH;
    if (0 == strcmp(method, "PUT")) return HTTP_PUT;
    return HTTP_INVALID;
}

void
ua_conn_add_header(struct ua_conn *conn,
                   const char field[],
                   const char value[])
{
    size_t fieldlen = strlen(field);
    struct curl_slist *node, *prev;
    char buf[4096];
    size_t buflen;
    char *ptr;

    buflen = snprintf(buf, sizeof(buf), "%s: %s", field, value);
    ASSERT_S(buflen < sizeof(buf), "Out of bounds write attempt");

    /* check for match in existing fields */
    for (prev = NULL, node = conn->header; node != NULL;
         prev = node, node = node->next)
    {
        if (!(ptr = strchr(node->data, ':')))
            ERR("Missing ':' in header:\n\t%s", node->data);

        if (fieldlen == (size_t)(ptr - node->data)
            && 0 == strncasecmp(node->data, field, fieldlen))
        {
            if (strlen(node->data) < buflen) {
                if (prev) prev->next = node->next;
                // XXX: since libcurl uses custom mallocs that offsets the data
                //      with some metadata, we rely on `curl_free()` to ensure
                //      proper behavior
                curl_free(node->data);
                curl_free(node);
                // XXX: this process can be optimized by completely replacing
                //      libcurl's `curl_slist_xxx()` functions with our own
                //      (see above comment)
                curl_slist_append(conn->header, buf);
            }
            else {
                memcpy(node->data, buf, buflen + 1);
            }

            return;
        }
    }

    /* couldn't find match, we will create a new field */
    if (NULL == conn->header)
        conn->header = curl_slist_append(NULL, buf);
    else
        curl_slist_append(conn->header, buf);
}

void
ua_conn_remove_header(struct ua_conn *conn, const char field[])
{
    size_t fieldlen = strlen(field);
    struct curl_slist *node, *prev = NULL;
    char *ptr;

    /* check for match in existing fields */
    for (node = conn->header; node != NULL; prev = node, node = node->next) {
        if (!(ptr = strchr(node->data, ':')))
            ERR("Missing ':' in header:\n\t%s", node->data);

        if (fieldlen == (size_t)(ptr - node->data)
            && 0 == strncasecmp(node->data, field, fieldlen))
        {
            if (!prev)
                conn->header = node->next;
            else
                prev->next = node->next;

            curl_free(node->data);
            curl_free(node);

            return;
        }
    }
}

char *
ua_conn_print_header(struct ua_conn *conn,
                     char *buf,
                     size_t bufsize,
                     struct ua_log_filter *log_filter)
{
    struct curl_slist *node;
    size_t ret = 0;

    for (node = conn->header; node != NULL; node = node->next) {
        const int header_name_size = strcspn(node->data, ":");
        int i = 0, hide_contents = 0;

        for (; i < log_filter->length; ++i) {
            if (header_name_size == log_filter->headers[i].size
                && 0
                       == strncasecmp(log_filter->headers[i].start, node->data,
                                      log_filter->headers[i].size))
            {
                hide_contents = 1;
                break;
            }
        }

        if (hide_contents)
            ret += snprintf(buf + ret, bufsize - ret, "%.*s: <<REDACTED>>\r\n",
                            header_name_size, node->data);
        else
            ret += snprintf(buf + ret, bufsize - ret, "%s\r\n", node->data);
        VASSERT_S(ret < bufsize, "[%s] Out of bounds write attempt",
                  conn->ua->conf.id);
    }
    if (!ret) return NULL;

    buf[ret - 1] = '\0';

    return buf;
}

/**
 * get http response header by lines
 * @see: https://curl.se/libcurl/c/CURLOPT_HEADERFUNCTION.html
 */
static size_t
_ua_conn_respheader_cb(char *buf, size_t size, size_t nmemb, void *p_userdata)
{
    struct ua_resp_header *header = p_userdata;
    size_t bufsize = size * nmemb;
    char *start = buf;
    char *end = buf + bufsize - 2; /* ignore \r\n */

    /* get ':' delimiter position */
    for (; buf != end && *buf != ':'; ++buf)
        continue;

    /* no ':' found means no field/value pair */
    if (*buf != ':') return bufsize;

    /* increase reusable header buffer only if necessary */
    if (header->bufsize < (header->len + bufsize + 1)) {
        header->bufsize = header->len + bufsize + 1;
        header->buf = realloc(header->buf, header->bufsize);
    }
    memcpy(&header->buf[header->len], start, bufsize);

    /* get the field part of the string */
    header->pairs[header->n_pairs].field.idx = header->len;
    header->pairs[header->n_pairs].field.size = buf - start;

    /* skip blank characters after ':' delimiter */
    for (buf += 1; buf != end && isspace(*buf); ++buf)
        continue;

    /* get the value part of the string */
    header->pairs[header->n_pairs].value.idx = header->len + (buf - start);
    header->pairs[header->n_pairs].value.size = (end - start) - (buf - start);

    header->len += bufsize;

    /* update amount of headers */
    ++header->n_pairs;

    ASSERT_S(header->n_pairs < UA_MAX_HEADER_PAIRS,
             "Out of bounds write attempt");

    return bufsize;
}

/**
 * get http response body in chunks
 * @see: https://curl.se/libcurl/c/CURLOPT_WRITEFUNCTION.html
 */
static size_t
_ua_conn_respbody_cb(char *buf, size_t size, size_t nmemb, void *p_userdata)
{
    struct ua_resp_body *body = p_userdata;
    size_t bufchunksize = size * nmemb;

    /* increase response body memory block size only if necessary */
    if (body->bufsize < (body->len + bufchunksize + 1)) {
        body->bufsize = body->len + bufchunksize + 1;
        body->buf = realloc(body->buf, body->bufsize);
    }
    memcpy(&body->buf[body->len], buf, bufchunksize);

    body->len += bufchunksize;
    body->buf[body->len] = '\0';

    return bufchunksize;
}

void
ua_set_opt(struct user_agent *ua,
           void *data,
           void (*callback)(struct ua_conn *conn, void *data))
{
    ua->setopt.callback = callback;
    ua->setopt.data = data;
}

void
ua_conn_set_mime(struct ua_conn *conn,
                 void *data,
                 void (*callback)(curl_mime *mime, void *data))
{
    conn->multipart.callback = callback;
    conn->multipart.data = data;
}

static struct ua_conn *
_ua_conn_init(struct user_agent *ua)
{
    static const char *user_agent =
        "Cogmasters (https://github.com/Cogmasters)";
    struct ua_conn *new_conn = calloc(1, sizeof(struct ua_conn));
    CURL *new_ehandle = curl_easy_init();

    /* default user agent */
    ua_conn_add_header(new_conn, "User-Agent", user_agent);

    /* set error buffer for capturing CURL error descriptions */
    curl_easy_setopt(new_ehandle, CURLOPT_ERRORBUFFER, new_conn->errbuf);
    /* set ptr to request header we will be using for API communication */
    curl_easy_setopt(new_ehandle, CURLOPT_HTTPHEADER, new_conn->header);
    /* enable follow redirections */
    curl_easy_setopt(new_ehandle, CURLOPT_FOLLOWLOCATION, 1L);
    /* set response body callback */
    curl_easy_setopt(new_ehandle, CURLOPT_WRITEFUNCTION,
                     &_ua_conn_respbody_cb);
    /* set ptr to response body to be filled at callback */
    curl_easy_setopt(new_ehandle, CURLOPT_WRITEDATA, &new_conn->info.body);
    /* set response header callback */
    curl_easy_setopt(new_ehandle, CURLOPT_HEADERFUNCTION,
                     &_ua_conn_respheader_cb);
    /* set ptr to response header to be filled at callback */
    curl_easy_setopt(new_ehandle, CURLOPT_HEADERDATA, &new_conn->info.header);

    new_conn->ehandle = new_ehandle;
    new_conn->ua = ua;

    /* additional easy handle setups with user callback */
    if (ua->setopt.callback) {
        ua->setopt.callback(new_conn, ua->setopt.data);
    }

    QUEUE_INIT(&new_conn->entry);

    return new_conn;
}

static void
_ua_conn_cleanup(struct ua_conn *conn)
{
    ua_info_cleanup(&conn->info);
    curl_easy_cleanup(conn->ehandle);
    if (conn->url.start) free(conn->url.start);
    if (conn->header) curl_slist_free_all(conn->header);
    free(conn);
}

struct ua_conn *
ua_conn_start(struct user_agent *ua)
{
    QUEUE(struct ua_conn) *qelem = NULL;
    struct ua_conn *conn = NULL;

    pthread_mutex_lock(&ua->connq->lock);

    if (QUEUE_EMPTY(&ua->connq->idle)) {
        conn = _ua_conn_init(ua);
        ++ua->connq->total;
    }
    else {
        /* remove from idle queue */
        qelem = QUEUE_HEAD(&ua->connq->idle);
        QUEUE_REMOVE(qelem);

        conn = QUEUE_DATA(qelem, struct ua_conn, entry);
    }
    QUEUE_INSERT_TAIL(&ua->connq->busy, &conn->entry);

    pthread_mutex_unlock(&ua->connq->lock);

    return conn;
}

static void
_ua_info_reset(struct ua_info *info)
{
    info->httpcode = 0;
    info->body.len = 0;
    info->header.len = 0;
    info->header.n_pairs = 0;
}

/* TODO: src should be 'struct ua_conn' */
static void
_ua_info_populate(struct ua_info *info, struct ua_conn *conn)
{
    struct logconf_szbuf logheader = { conn->info.header.buf,
                                       conn->info.header.len };
    struct logconf_szbuf logbody = { conn->info.body.buf,
                                     conn->info.body.len };
    char *resp_url = NULL;

    memcpy(info, &conn->info, sizeof(struct ua_info));

    info->body.len = cog_strndup(logbody.start, logbody.size, &info->body.buf);
    info->header.len =
        cog_strndup(logheader.start, logheader.size, &info->header.buf);

    /* get response's code */
    curl_easy_getinfo(conn->ehandle, CURLINFO_RESPONSE_CODE, &info->httpcode);
    /* get response's url */
    curl_easy_getinfo(conn->ehandle, CURLINFO_EFFECTIVE_URL, &resp_url);

    logconf_http(&conn->ua->conf, &conn->info.loginfo, resp_url, logheader,
                 logbody, "HTTP_RCV_%s(%ld)", http_code_print(info->httpcode),
                 info->httpcode);
}

void
ua_conn_reset(struct ua_conn *conn)
{
    /* reset conn fields for next iteration */
    _ua_info_reset(&conn->info);
    *conn->errbuf = '\0';
}

void
ua_conn_stop(struct ua_conn *conn)
{
    struct user_agent *ua = conn->ua;

    ua_conn_reset(conn);

    if (conn->multipart.mime) {
        curl_mime_free(conn->multipart.mime);
        conn->multipart.mime = NULL;
    }

    /* move conn from 'busy' to 'idle' queue */
    pthread_mutex_lock(&ua->connq->lock);
    QUEUE_REMOVE(&conn->entry);
    QUEUE_INSERT_TAIL(&ua->connq->idle, &conn->entry);
    pthread_mutex_unlock(&ua->connq->lock);
}

struct user_agent *
ua_init(struct ua_attr *attr)
{
    struct user_agent *new_ua = calloc(1, sizeof *new_ua);

    logconf_branch(&new_ua->conf, attr ? attr->conf : NULL, "USER_AGENT");

    new_ua->connq = calloc(1, sizeof *new_ua->connq);
    QUEUE_INIT(&new_ua->connq->idle);
    QUEUE_INIT(&new_ua->connq->busy);

    if (pthread_mutex_init(&new_ua->connq->lock, NULL)) {
        logconf_fatal(&new_ua->conf, "Couldn't initialize mutex");
        abort();
    }
    return new_ua;
}

void
ua_cleanup(struct user_agent *ua)
{
    QUEUE *const ua_queues[] = { &ua->connq->idle, &ua->connq->busy };

    /* cleanup connection queues */
    for (size_t i = 0; i < sizeof(ua_queues) / sizeof *ua_queues; ++i) {
        QUEUE(struct ua_conn) queue, *qelem;
        struct ua_conn *conn;

        QUEUE_MOVE(ua_queues[i], &queue);
        while (!QUEUE_EMPTY(&queue)) {
            qelem = QUEUE_HEAD(&queue);
            QUEUE_REMOVE(qelem);

            conn = QUEUE_DATA(qelem, struct ua_conn, entry);
            _ua_conn_cleanup(conn);
        }
    }
    pthread_mutex_destroy(&ua->connq->lock);
    free(ua->connq);

    /* cleanup logging module */
    logconf_cleanup(&ua->conf);

    /* cleanup base URL */
    if (ua->base_url.start) free(ua->base_url.start);

    /* cleanup User-Agent handle */
    free(ua);
}

const char *
ua_get_url(struct user_agent *ua)
{
    return ua->base_url.start;
}

void
ua_set_url(struct user_agent *ua, const char base_url[])
{
    if (ua->base_url.start) free(ua->base_url.start);
    ua->base_url.size =
        cog_strndup(base_url, strlen(base_url), &ua->base_url.start);
}

/* set specific http method used for the request */
static void
_ua_conn_set_method(struct ua_conn *conn,
                    enum http_method method,
                    char *body,
                    size_t body_size,
                    struct ua_log_filter *log_filter)
{
    char logbuf[1024] = "";
    struct logconf_szbuf logheader = { logbuf, sizeof(logbuf) };
    struct logconf_szbuf logbody = { body, body_size };
    const char *method_str = http_method_print(method);
    struct logconf *conf = &conn->ua->conf;

    ua_conn_print_header(conn, logbuf, sizeof(logbuf), log_filter);

    /* make sure body points to something */
    if (!body) body = "";

    logconf_http(conf, &conn->info.loginfo, conn->url.start, logheader,
                 logbody, "HTTP_SEND_%s", method_str);

    logconf_trace(conf, ANSICOLOR("SEND", ANSI_FG_GREEN) " %s [@@@_%zu_@@@]",
                  method_str, conn->info.loginfo.rcounter);

    /* resets any preexisting CUSTOMREQUEST */
    curl_easy_setopt(conn->ehandle, CURLOPT_CUSTOMREQUEST, NULL);

    switch (method) {
    case HTTP_DELETE:
        curl_easy_setopt(conn->ehandle, CURLOPT_CUSTOMREQUEST, "DELETE");
        break;
    case HTTP_GET:
        curl_easy_setopt(conn->ehandle, CURLOPT_HTTPGET, 1L);
        return;
    case HTTP_POST:
        curl_easy_setopt(conn->ehandle, CURLOPT_POST, 1L);
        break;
    case HTTP_MIMEPOST:
        ASSERT_S(NULL != conn->multipart.callback,
                 "Missing 'ua_conn_set_mime()' callback");
        ASSERT_S(NULL == conn->multipart.mime, "Previous 'mime' not freed");

        conn->multipart.mime = curl_mime_init(conn->ehandle);
        conn->multipart.callback(conn->multipart.mime, conn->multipart.data);

        curl_easy_setopt(conn->ehandle, CURLOPT_MIMEPOST,
                         conn->multipart.mime);
        return;
    case HTTP_PATCH:
        curl_easy_setopt(conn->ehandle, CURLOPT_CUSTOMREQUEST, "PATCH");
        break;
    case HTTP_PUT:
        curl_easy_setopt(conn->ehandle, CURLOPT_CUSTOMREQUEST, "PUT");
        break;
    default:
        logconf_fatal(&conn->ua->conf, "Unknown http method (code: %d)",
                      method);
        abort();
    }

    /* set ptr to payload that will be sent via POST/PUT/PATCH */
    curl_easy_setopt(conn->ehandle, CURLOPT_POSTFIELDSIZE, body_size);
    curl_easy_setopt(conn->ehandle, CURLOPT_POSTFIELDS, body);
}

/* combine base url with endpoint and assign it to 'conn' */
static void
_ua_conn_set_url(struct ua_conn *conn, char base_url[], char endpoint[])
{
    size_t size = 2;
    CURLcode ecode;
    size_t ret;

    if (!base_url) {
        base_url = conn->ua->base_url.start;
        size += conn->ua->base_url.size;
    }
    else {
        size += strlen(base_url);
    }

    if (!endpoint)
        endpoint = "";
    else
        size += strlen(endpoint);

    /* increase buffer length if necessary */
    if (size > conn->url.size) {
        void *tmp = realloc(conn->url.start, size);
        ASSERT_S(NULL != tmp, "Couldn't increase buffer's length");

        conn->url.start = tmp;
        conn->url.size = size;
    }

    /* append endpoint to base url */
    ret =
        snprintf(conn->url.start, conn->url.size, "%s%s", base_url, endpoint);
    ASSERT_S(ret < conn->url.size, "Out of bounds write attempt");

    logconf_trace(&conn->ua->conf, "Request URL: %s", conn->url.start);

    /* assign url to conn's easy handle */
    ecode = curl_easy_setopt(conn->ehandle, CURLOPT_URL, conn->url.start);
    if (ecode != CURLE_OK) CURLE_LOG(conn, ecode);
}

void
ua_conn_setup(struct ua_conn *conn, struct ua_conn_attr *attr)
{
    _ua_conn_set_url(conn, attr->base_url, attr->endpoint);
    _ua_conn_set_method(conn, attr->method, attr->body, attr->body_size,
                        &attr->log_filter);
}

/* get request results */
CCORDcode
ua_info_extract(struct ua_conn *conn, struct ua_info *info)
{
    _ua_info_populate(info, conn);

    /* triggers response callbacks */
    if (info->httpcode >= 500 && info->httpcode < 600) {
        logconf_error(&conn->ua->conf,
                      ANSICOLOR("SERVER ERROR",
                                ANSI_FG_RED) " (%d)%s - %s [@@@_%zu_@@@]",
                      info->httpcode, http_code_print(info->httpcode),
                      http_reason_print(info->httpcode),
                      info->loginfo.rcounter);

        info->code = CCORD_HTTP_CODE;
    }
    else if (info->httpcode >= 400) {
        logconf_error(&conn->ua->conf,
                      ANSICOLOR("CLIENT ERROR",
                                ANSI_FG_RED) " (%d)%s - %s [@@@_%zu_@@@]",
                      info->httpcode, http_code_print(info->httpcode),
                      http_reason_print(info->httpcode),
                      info->loginfo.rcounter);

        info->code = CCORD_HTTP_CODE;
    }
    else if (info->httpcode >= 300) {
        logconf_warn(&conn->ua->conf,
                     ANSICOLOR("REDIRECTING",
                               ANSI_FG_YELLOW) " (%d)%s - %s [@@@_%zu_@@@]",
                     info->httpcode, http_code_print(info->httpcode),
                     http_reason_print(info->httpcode),
                     info->loginfo.rcounter);

        info->code = CCORD_HTTP_CODE;
    }
    else if (info->httpcode >= 200) {
        logconf_info(
            &conn->ua->conf,
            ANSICOLOR("SUCCESS", ANSI_FG_GREEN) " (%d)%s - %s [@@@_%zu_@@@]",
            info->httpcode, http_code_print(info->httpcode),
            http_reason_print(info->httpcode), info->loginfo.rcounter);

        info->code = CCORD_OK;
    }
    else if (info->httpcode >= 100) {
        logconf_info(
            &conn->ua->conf,
            ANSICOLOR("INFO", ANSI_FG_GRAY) " (%d)%s - %s [@@@_%zu_@@@]",
            info->httpcode, http_code_print(info->httpcode),
            http_reason_print(info->httpcode), info->loginfo.rcounter);

        info->code = CCORD_HTTP_CODE;
    }
    else if (info->httpcode > 0) {
        logconf_error(&conn->ua->conf, "Unusual HTTP response code: %d",
                      info->httpcode);

        info->code = CCORD_UNUSUAL_HTTP_CODE;
    }
    else {
        logconf_error(&conn->ua->conf, "No http response received by libcurl");

        info->code = CCORD_CURL_NO_RESPONSE;
    }

    return info->code;
}

CURL *
ua_conn_get_easy_handle(struct ua_conn *conn)
{
    return conn->ehandle;
}

CCORDcode
ua_conn_easy_perform(struct ua_conn *conn)
{
    CURLcode ecode;

    ecode = curl_easy_perform(conn->ehandle);
    if (ecode != CURLE_OK) {
        CURLE_LOG(conn, ecode);
        return CCORD_CURLE_INTERNAL;
    }
    return CCORD_OK;
}

/* template function for performing blocking requests */
CCORDcode
ua_easy_run(struct user_agent *ua,
            struct ua_info *info,
            struct ua_resp_handle *handle,
            struct ua_conn_attr *attr)
{
    struct ua_conn *conn = ua_conn_start(ua);
    CCORDcode code;

    /* populate conn with parameters */
    if (attr) ua_conn_setup(conn, attr);

    /* perform blocking request, and check results */
    if (CCORD_OK == (code = ua_conn_easy_perform(conn))) {
        struct ua_info _info = { 0 };

        code = ua_info_extract(conn, &_info);

        if (handle) {
            if (_info.httpcode >= 400 && _info.httpcode < 600) {
                if (handle->err_cb)
                    handle->err_cb(_info.body.buf, _info.body.len,
                                   handle->err_obj);
            }
            else if (_info.httpcode >= 200 && _info.httpcode < 300) {
                if (handle->ok_cb)
                    handle->ok_cb(_info.body.buf, _info.body.len,
                                  handle->ok_obj);
            }
        }

        if (info)
            memcpy(info, &_info, sizeof(struct ua_info));
        else
            ua_info_cleanup(&_info);
    }

    /* reset conn and mark it as free to use */
    ua_conn_stop(conn);

    return code;
}

void
ua_info_cleanup(struct ua_info *info)
{
    if (info->body.buf) free(info->body.buf);
    if (info->header.buf) free(info->header.buf);
    memset(info, 0, sizeof(struct ua_info));
}

/** attempt to get value from matching response header field */
struct ua_szbuf_readonly
ua_info_get_header(struct ua_info *info, char field[])
{
    size_t len = strlen(field);
    struct ua_szbuf_readonly value;
    int i;

    for (i = 0; i < info->header.n_pairs; ++i) {
        struct ua_szbuf_readonly header = {
            info->header.buf + info->header.pairs[i].field.idx,
            info->header.pairs[i].field.size,
        };

        if (len == header.size && 0 == strncasecmp(field, header.start, len)) {
            /* found field match, get value */
            value.start = info->header.buf + info->header.pairs[i].value.idx;
            value.size = info->header.pairs[i].value.size;

            return value;
        }
    }

    /* couldn't match field */
    value.start = NULL;
    value.size = 0;

    return value;
}

struct ua_szbuf_readonly
ua_info_get_body(struct ua_info *info)
{
    struct ua_szbuf_readonly body = { info->body.buf, info->body.len };

    return body;
}
