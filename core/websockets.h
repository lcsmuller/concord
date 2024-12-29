/**
 * @file websockets.h
 */

#ifndef WEBSOCKETS_H
#define WEBSOCKETS_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <curl/curl.h>

#include "error.h" /* CCORDcode */
#include "logconf.h" /* logging facilities */

/**
 * @struct websockets
 * @brief Opaque handler for WebSockets
 *
 * @see ws_init(), ws_cleanup()
 */
struct websockets;

/**
 * @brief Stores info on the latest transfer performed via websockets
 */
struct ws_info {
    /** logging info */
    struct loginfo loginfo;
    /** how the transfer went @todo implement */
    CCORDcode code;
};

/**
 * @brief The WebSockets client status
 *
 * @see ws_get_status()
 */
enum ws_status {
    /** client disconnected from ws */
    WS_DISCONNECTED = 0,
    /** client connected to ws */
    WS_CONNECTED,
    /** client in the process of disconnecting from ws */
    WS_DISCONNECTING,
    /** client in the process of connecting to ws */
    WS_CONNECTING,
};

/**
 * @brief WebSockets CLOSE opcodes
 * @see ws_close_opcode_print()
 * @see https://tools.ietf.org/html/rfc6455#section-7.4.1
 */
enum ws_close_reason {
    WS_CLOSE_REASON_NORMAL = 1000,
    WS_CLOSE_REASON_GOING_AWAY = 1001,
    WS_CLOSE_REASON_PROTOCOL_ERROR = 1002,
    WS_CLOSE_REASON_UNEXPECTED_DATA = 1003,
    WS_CLOSE_REASON_NO_REASON = 1005,
    WS_CLOSE_REASON_ABRUPTLY = 1006,
    WS_CLOSE_REASON_INCONSISTENT_DATA = 1007,
    WS_CLOSE_REASON_POLICY_VIOLATION = 1008,
    WS_CLOSE_REASON_TOO_BIG = 1009,
    WS_CLOSE_REASON_MISSING_EXTENSION = 1010,
    WS_CLOSE_REASON_SERVER_ERROR = 1011,
    WS_CLOSE_REASON_IANA_REGISTRY_START = 3000,
    WS_CLOSE_REASON_IANA_REGISTRY_END = 3999,
    WS_CLOSE_REASON_PRIVATE_START = 4000,
    WS_CLOSE_REASON_PRIVATE_END = 4999
};

/** @brief WebSockets callbacks */
struct ws_callbacks {
    /**
     * @brief Called upon connection
     */
    void (*on_connect)(void *data,
                       struct websockets *ws,
                       struct ws_info *info);

    /**
     * @brief Reports UTF-8 text messages.
     *
     * @note it's guaranteed to be NULL (\0) terminated, but the UTF-8 is
     * not validated. If it's invalid, consider closing the connection
     * with WS_CLOSE_REASON_INCONSISTENT_DATA.
     */
    void (*on_text)(void *data,
                    struct websockets *ws,
                    struct ws_info *info,
                    const char *text,
                    size_t len);

    /** @brief reports binary data.  */
    void (*on_binary)(void *data,
                      struct websockets *ws,
                      struct ws_info *info,
                      const void *mem,
                      size_t len);
    /**
     * @brief reports PING.
     *
     * @note if provided you should reply with ws_pong(). If not
     * provided, pong is sent with the same message payload.
     */
    void (*on_ping)(void *data,
                    struct websockets *ws,
                    struct ws_info *info,
                    const char *reason,
                    size_t len);

    /** @brief reports PONG.  */
    void (*on_pong)(void *data,
                    struct websockets *ws,
                    struct ws_info *info,
                    const char *reason,
                    size_t len);

    /**
     * @brief reports server closed the connection with the given reason.
     *
     * Clients should not transmit any more data after the server is
     * closed
     */
    void (*on_close)(void *data,
                     struct websockets *ws,
                     struct ws_info *info,
                     enum ws_close_reason wscode,
                     const char *reason,
                     size_t len);

    /** @brief user arbitrary data to be passed around callbacks */
    void *data;
};

/** @brief WebSockets handle initialization attributes */
struct ws_attr {
    /** pre-initialized logging module */
    struct logconf *conf;
};

/**
 * @brief Check if a WebSockets connection is alive
 *
 * This will only return true if the connection status is
 *        different than WS_DISCONNECTED
 * @param ws the WebSockets handle created with ws_init()
 * @return `true` if WebSockets status is different than
 *        WS_DISCONNECTED, `false` otherwise.
 */
#define ws_is_alive(ws) (ws_get_status(ws) != WS_DISCONNECTED)

/**
 * @brief Check if WebSockets connection is functional
 *
 * This will only return true if the connection status is
 *        WS_CONNECTED
 * @param ws the WebSockets handle created with ws_init()
 * @return `true` if is functional, `false` otherwise
 */
#define ws_is_functional(ws) (ws_get_status(ws) == WS_CONNECTED)

/**
 * @brief Create a new (CURL-based) WebSockets handle
 *
 * @param cbs set of functions to call back when server report events.
 * @param mhandle user-owned curl_multi handle for performing non-blocking
 * transfers
 * @param attr optional attributes to override defaults
 * @return newly created WebSockets handle, free with ws_cleanup()
 */
struct websockets *ws_init(struct ws_callbacks *cbs,
                           CURLM *mhandle,
                           struct ws_attr *attr);

/**
 * @brief Free a WebSockets handle created with ws_init()
 *
 * @param ws the WebSockets handle created with ws_init()
 */
void ws_cleanup(struct websockets *ws);

/**
 * @brief Set the URL for the WebSockets handle to connect
 *
 * @param ws the WebSockets handle created with ws_init()
 * @param base_url the URL to connect, such as ws://echo.websockets.org
 */
void ws_set_url(struct websockets *ws,
                const char base_url[]);

/**
 * @brief Send a binary message of given size.
 *
 * Binary messages do not need to include the null terminator (\0), they
 * will be read up to @a msglen.
 *
 * @param ws the WebSockets handle created with ws_init()
 * @param info get information on how this transfer went
 * @param msg the pointer to memory (linear) to send.
 * @param msglen the length in bytes of @a msg.
 * @return true if sent, false on errors.
 */
_Bool ws_send_binary(struct websockets *ws,
                     struct ws_info *info,
                     const char msg[],
                     size_t msglen);
/**
 * @brief Send a text message of given size.
 *
 * Text messages do not need to include the null terminator (\0), they
 * will be read up to @a len.
 *
 * @param ws the WebSockets handle created with ws_init()
 * @param info get information on how this transfer went
 * @param text the pointer to memory (linear) to send.
 * @param len the length in bytes of @a text.
 * @return true if sent, false on errors.
 */
_Bool ws_send_text(struct websockets *ws,
                   struct ws_info *info,
                   const char text[],
                   size_t len);
/**
 * @brief Send a PING (opcode 0x9) frame with @a reason as payload.
 *
 * @param ws the WebSockets handle created with ws_init()
 * @param info get information on how this transfer went
 * @param reason NULL or some UTF-8 string null ('\0') terminated.
 * @param len the length of @a reason in bytes. If SIZE_MAX, uses
 *        strlen() on @a reason if it's not NULL.
 * @return true if sent, false on errors.
 */
_Bool ws_ping(struct websockets *ws,
              struct ws_info *info,
              const char reason[],
              size_t len);
/**
 * @brief Send a PONG (opcode 0xA) frame with @a reason as payload.
 *
 * Note that pong is sent automatically if no "on_ping" callback is
 * defined. If one is defined you must send pong manually.
 *
 * @param ws the WebSockets handle created with ws_init()
 * @param info get information on how this transfer went
 * @param reason NULL or some UTF-8 string null ('\0') terminated.
 * @param len the length of @a reason in bytes. If SIZE_MAX, uses
 *        strlen() on @a reason if it's not NULL.
 * @return true if sent, false on errors.
 */
_Bool ws_pong(struct websockets *ws,
              struct ws_info *info,
              const char reason[],
              size_t len);

/**
 * @brief Signals connecting state before entering the WebSockets event loop
 *
 * @param ws the WebSockets handle created with ws_init()
 * @return the WebSockets easy_handle that is free'd at ws_end()
 */
CURL* ws_start(struct websockets *ws);

/**
 * @brief Cleanup and reset `ws` connection resources
 *
 * @param ws the WebSockets handle created with ws_init()
 */
void ws_end(struct websockets *ws);

/**
 * @brief Reads/Write available data from WebSockets
 * @note Helper over curl_multi_wait()
 *
 * @param ws the WebSockets handle created with ws_init()
 * @param wait_ms limit amount in milliseconds to wait for until activity
 * @param tstamp get current timestamp for this iteration
 * @return `true` if connection is still alive, `false` otherwise
 * @note This is an easy, yet highly abstracted way of performing transfers.
 *        If a higher control is necessary, users are better of using
 * ws_multi_socket_run()
 */
_Bool ws_easy_run(struct websockets *ws, uint64_t wait_ms, uint64_t *tstamp);

/**
 * @brief Reads/Write available data from WebSockets
 * @note Helper over curl_multi_socket_all()
 *
 * @param ws the WebSockets handle created with ws_init()
 * @param tstamp get current timestamp for this iteration
 * @return `true` if connection is still alive, `false` otherwise
 */
_Bool ws_multi_socket_run(struct websockets *ws, uint64_t *tstamp);

/**
 * @brief Returns the WebSockets handle connection status
 *
 * @param ws the WebSockets handle created with ws_init()
 * @return a ws_status opcode
 */
enum ws_status ws_get_status(struct websockets *ws);

/**
 * @brief Returns a enum ws_close_reason opcode in a string format
 *
 * @param opcode the opcode to be converted to string
 * @return a read-only string literal of the opcode
 */
const char *ws_close_opcode_print(enum ws_close_reason opcode);

/**
 * @brief The WebSockets event-loop concept of "now"
 *
 * @param ws the WebSockets handle created with ws_init()
 * @return the timestamp in milliseconds from when ws_timestamp_update() was
 * last called
 * @note the timestamp is updated at the start of each event-loop iteration
 */
uint64_t ws_timestamp(struct websockets *ws);

/**
 * @brief Update the WebSockets event-loop concept of "now"
 *
 * @param ws the WebSockets handle created with ws_init()
 * @return the timestamp in milliseconds
 */
uint64_t ws_timestamp_update(struct websockets *ws);

/**
 * @brief Thread-safe way to stop websockets connection
 *
 * This will activate a internal WS_USER_CMD_EXIT flag that will
 *        force disconnect when the next iteration begins.
 * @note it will create a copy of the reason string
 * @param ws the WebSockets handle created with ws_init()
 * @param code the WebSockets CLOSE opcode
 * @param reason the close reason
 * @param len the reason length
 */
void ws_close(struct websockets *ws,
              const enum ws_close_reason code,
              const char reason[],
              const size_t len);

/**
 * @brief Add a header field/value pair
 *
 * @param ws the WebSockets handle created with ws_init()
 * @param field the header field
 * @param value the header value
 */
void ws_add_header(struct websockets *ws,
                   const char field[],
                   const char value[]);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* WEBSOCKETS_H */
