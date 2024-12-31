#ifndef CONCORD_IO_POLLER_H
#define CONCORD_IO_POLLER_H

#include <stdbool.h>
#include <curl/curl.h>

/**
 * @brief The flags to poll for
 */
enum io_poller_events {
  IO_POLLER_IN  = 1 << 0,
  IO_POLLER_OUT = 1 << 1,
  IO_POLLER_PRI = 1 << 2,
  IO_POLLER_ERR = 1 << 3,
};

/**
 * @brief a socket or file descriptor
 */
typedef int io_poller_socket;

/**
 * @brief handle for watching file descriptors, sockets, and curl multis
 */
struct io_poller;

/**
 * @brief callback for when an event is triggered by the socket
 */
typedef void (*io_poller_cb)(struct io_poller *io,
                             enum io_poller_events events,
                             void *user_data);

struct io_poller *io_poller_create(void);
void io_poller_destroy(struct io_poller *io);

/**
 * @brief wakeup the thread listening to this io_poller
 * 
 * @param io the io_poller to wake up
 */
void
io_poller_wakeup(struct io_poller *io);

/**
 * @brief wait for events to be triggered
 * @param io the io_poller to poll on
 * @param milliseconds -1 for infinity, or ms to poll for
 * @return -1 for error, or number of sockets that have events waiting
 */
int io_poller_poll(struct io_poller *io, int milliseconds);

/**
 * @brief performs any actions needed and clears events set by io_poller_poll
 * @param io the io_poller to perform on
 * @return 0 on success
 */
int io_poller_perform(struct io_poller *io);

/**
 * @brief adds or modifies a socket or file descriptor to watch list
 * @param io the io_poller to add socket to
 * @param sock the file descriptor or socket to handle
 * @param events the events to watch for
 * @param cb the callback for when any event is triggered
 * @param user_data custom user data
 * @return true on success
 */
bool io_poller_socket_add(struct io_poller *io,
                          io_poller_socket sock,
                          enum io_poller_events events,
                          io_poller_cb cb,
                          void *user_data);

/**
 * @brief removes a socket or file descriptor from watch list
 * @param io the io_poller to remove the socket from
 * @param sock the file descriptor or socket to remove
 * @return true on success
 */
bool io_poller_socket_del(struct io_poller *io, io_poller_socket sock);

/**
 * @brief callback for when curl multi should be performed on
 */
typedef int (*io_poller_curl_cb)(struct io_poller *io,
                                 CURLM *multi,
                                 void *user_data);

/**
 * @brief add or modifies a curl multi to watch list
 * @param io the io_poller to add curl multi to
 * @param multi the curl multi to add or modify
 * @param cb the callback for when curl multi should be performed on
 * @param user_data custom user data
 * @return true on success
 */
bool io_poller_curlm_add(struct io_poller *io,
                         CURLM *multi,
                         io_poller_curl_cb cb,
                         void *user_data);

/**
 * @brief remove curl multi from watch list
 * @param io the io_poller to remove curl multi from
 * @param multi the curl multi to remove
 * @return true on success
 */
bool io_poller_curlm_del(struct io_poller *io, CURLM *multi);

/**
 * @brief this multi should be performed on next cycle
 * causing poll to return immediately
 * @param io the io_poller to enable perform on
 * @param multi the multi that should be performed
 * @return true on success
 */
bool io_poller_curlm_enable_perform(struct io_poller *io, CURLM *multi);

#endif // CONCORD_IO_POLLER_H
