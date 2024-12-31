#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>
#include <sys/ioctl.h>

#ifndef __MINGW32__
#include <poll.h>
#endif

#include "io_poller.h"
#include "cog-utils.h"

struct io_curlm {
    CURLM *multi;
    struct io_poller *io_poller;
    int *fds;
    int fds_cnt;
    int fds_cap;
    io_poller_curl_cb cb;
    int64_t timeout;
    void *user_data;
    int running;
    bool should_perform;
};

struct io_poller_element {
    void *user_data;
    io_poller_cb cb;
};

struct io_poller {
    struct io_poller_element *elements;
    struct pollfd *pollfds;
    int cap;
    int cnt;

    struct io_curlm **curlm;
    int curlm_cap;
    int curlm_cnt;

    int wakeup_fds[2];
};

static void
on_io_poller_wakeup(struct io_poller *io,
                    enum io_poller_events events,
                    void *user_data)
{
    char buf[0x1000];
    (void)!read(io->wakeup_fds[0], buf, sizeof buf);
}

struct io_poller *
io_poller_create(void)
{
    struct io_poller *io = calloc(1, sizeof *io);
    if (io) {
        io->cap = 0x10;
        io->elements = calloc(io->cap, sizeof *io->elements);
        io->pollfds = calloc(io->cap, sizeof *io->pollfds);
        if (io->elements && io->pollfds) {
            if (0 == pipe(io->wakeup_fds)) {
                bool success = true;
                for (int i = 0; i < 2; i++) {
                    int flags = fcntl(io->wakeup_fds[i], F_GETFL) | O_NONBLOCK;
                    if (0 != fcntl(io->wakeup_fds[i], F_SETFL, flags))
                        success = false;
#ifdef FIOCLEX
                    if (0 != ioctl(io->wakeup_fds[i], FIOCLEX, NULL))
                        success = false;
#endif
                }
                if (success) {
                    io_poller_socket_add(io, io->wakeup_fds[0], IO_POLLER_IN,
                                         on_io_poller_wakeup, NULL);
                    return io;
                }
                close(io->wakeup_fds[1]);
                close(io->wakeup_fds[0]);
            }
        }
        free(io->elements);
        free(io->pollfds);
        free(io);
    }
    return NULL;
}

void
io_poller_destroy(struct io_poller *io)
{
    close(io->wakeup_fds[0]);
    close(io->wakeup_fds[1]);
    for (int i = 0; i < io->curlm_cnt; i++) {
        free(io->curlm[i]->fds);
        free(io->curlm[i]);
    }
    free(io->curlm);
    free(io->elements);
    free(io->pollfds);
    free(io);
}

void
io_poller_wakeup(struct io_poller *io)
{
    char buf = 0;
    (void)!write(io->wakeup_fds[1], &buf, sizeof buf);
}

int
io_poller_poll(struct io_poller *io, int milliseconds)
{
    const int64_t now = cog_timestamp_ms();
    const int64_t millis = now + (milliseconds == -1 ? 0 : milliseconds);
    for (int i = 0; i < io->curlm_cnt; i++) {
        struct io_curlm *io_curlm = io->curlm[i];
        if (io_curlm->should_perform) {
            milliseconds = 0;
            break;
        }
        if (io_curlm->timeout != -1)
            if (milliseconds == -1 || io_curlm->timeout < millis)
                milliseconds =
                    io_curlm->timeout < now ? 0 : io_curlm->timeout - now;
    }
    return poll(io->pollfds, io->cnt, milliseconds);
}

int
io_poller_perform(struct io_poller *io)
{
    const int64_t now = cog_timestamp_ms();
    for (int i = 0; i < io->cnt; i++) {
        if (io->pollfds[i].revents) {
            int events = 0;
            if (io->pollfds[i].revents & POLLIN) events |= IO_POLLER_IN;
            if (io->pollfds[i].revents & POLLOUT) events |= IO_POLLER_OUT;
            if (io->pollfds[i].revents & POLLPRI) events |= IO_POLLER_PRI;
            if (io->pollfds[i].revents & POLLERR) events |= IO_POLLER_ERR;
            io->pollfds[i].revents = 0;
            struct io_poller_element *element = &io->elements[i];
            element->cb(io, events, element->user_data);
        }
    }
    for (int i = 0; i < io->curlm_cnt; i++) {
        struct io_curlm *curlm = io->curlm[i];
        if (curlm->should_perform
            || (-1 != curlm->timeout && now >= curlm->timeout))
        {
            curlm->should_perform = false;
            int result =
                curlm->cb
                    ? curlm->cb(io, curlm->multi, curlm->user_data)
                    : curl_multi_socket_all(curlm->multi, &curlm->running);

            if (result != 0) return result;
        }
    }
    return 0;
}

bool
io_poller_socket_add(struct io_poller *io,
                     io_poller_socket fd,
                     enum io_poller_events events,
                     io_poller_cb cb,
                     void *user_data)
{
    int index = 0;
    for (; index < io->cnt; index++)
        if (fd == io->pollfds[index].fd) goto modify;

    if (io->cnt == io->cap) {
        size_t cap = io->cap << 1;
        void *tmp;

        tmp = realloc(io->pollfds, cap * sizeof *io->pollfds);
        if (!tmp) return false;
        io->pollfds = tmp;

        tmp = realloc(io->elements, cap * sizeof *io->elements);
        if (!tmp) return false;
        io->elements = tmp;

        memset(&io->elements[io->cap], 0,
               (cap - io->cap) * sizeof *io->elements);
        memset(&io->pollfds[io->cap], 0,
               (cap - io->cap) * sizeof *io->pollfds);
        io->cap = cap;
    }

    io->cnt++;
    io->pollfds[index].fd = fd;

modify:
    io->pollfds[index].events = 0;
    if (events & IO_POLLER_IN) io->pollfds[index].events |= POLLIN;
    if (events & IO_POLLER_OUT) io->pollfds[index].events |= POLLOUT;
    if (events & IO_POLLER_PRI) io->pollfds[index].events |= POLLPRI;
    io->elements[index].cb = cb;
    io->elements[index].user_data = user_data;
    return true;
}

bool
io_poller_socket_del(struct io_poller *io, io_poller_socket fd)
{
    int index = 0;
    for (; index < io->cnt; index++)
        if (fd == io->pollfds[index].fd) goto modify;
    return false;

modify:
    if (index < --io->cnt) {
        memcpy(&io->pollfds[index], &io->pollfds[io->cnt],
               sizeof *io->pollfds);
        memcpy(&io->elements[index], &io->elements[io->cnt],
               sizeof *io->elements);
    }
    memset(&io->pollfds[io->cnt], 0, sizeof *io->pollfds);
    memset(&io->elements[io->cnt], 0, sizeof *io->elements);
    return true;
}

static void
io_curl_cb(struct io_poller *io, enum io_poller_events events, void *user_data)
{
    (void)io;
    (void)events;
    struct io_curlm *io_curlm = user_data;
    io_curlm->should_perform = true;
}

static int
curl_socket_cb(
    CURL *easy, curl_socket_t fd, int what, void *userp, void *socketp)
{
    (void)easy;
    (void)socketp;
    struct io_curlm *io_curlm = userp;
    int index = -1;
    for (int i = 0; i < io_curlm->fds_cnt; i++) {
        if (io_curlm->fds[i] == fd) {
            index = i;
            break;
        }
    }

    enum io_poller_events events = 0;
    switch (what) {
    case CURL_POLL_IN:
        events = IO_POLLER_IN;
        break;
    case CURL_POLL_OUT:
        events = IO_POLLER_OUT;
        break;
    case CURL_POLL_INOUT:
        events = IO_POLLER_IN | IO_POLLER_OUT;
        break;
    case CURL_POLL_REMOVE:
        io_poller_socket_del(io_curlm->io_poller, fd);
        if (index != -1)
            memmove(&io_curlm->fds[index], &io_curlm->fds[index + 1],
                    (--io_curlm->fds_cnt - index) * sizeof *io_curlm->fds);
        return 0;
    }
    if (index == -1) {
        if (io_curlm->fds_cnt == io_curlm->fds_cap) {
            int cap = io_curlm->fds_cap << 1;
            if (!cap) cap = 8;
            void *tmp = realloc(io_curlm->fds, cap * sizeof *io_curlm->fds);
            if (tmp) {
                io_curlm->fds = tmp;
                io_curlm->fds_cap = cap;
            }
            else
                return CURLM_OUT_OF_MEMORY;
        }
        io_curlm->fds[io_curlm->fds_cnt++] = fd;
    }
    io_poller_socket_add(io_curlm->io_poller, fd, events, io_curl_cb,
                         io_curlm);
    return CURLM_OK;
}

static int
curl_timer_cb(CURLM *multi, long timeout_ms, void *userp)
{
    (void)multi;
    struct io_curlm *io_curlm = userp;
    if (timeout_ms == -1) {
        io_curlm->timeout = -1;
    }
    else {
        io_curlm->timeout = cog_timestamp_ms() + timeout_ms;
    }
    return CURLM_OK;
}

bool
io_poller_curlm_add(struct io_poller *io,
                    CURLM *multi,
                    io_poller_curl_cb cb,
                    void *user_data)
{
    struct io_curlm *io_curlm = NULL;
    size_t index = 0;
    for (; index < io->curlm_cnt; index++) {
        if (io->curlm[index]->multi == multi) {
            io_curlm = io->curlm[index];
            goto modify;
        }
    }
    if (io->curlm_cnt == io->curlm_cap) {
        size_t cap = io->curlm_cap << 1;
        if (!cap) cap = 8;
        void *tmp = realloc(io->curlm, cap * sizeof *io->curlm);
        if (!tmp) return false;
        io->curlm = tmp;
        io->curlm_cap = cap;
    }

    if (!(io_curlm = calloc(1, sizeof *io_curlm))) return false;
    io->curlm[io->curlm_cnt++] = io_curlm;
    io_curlm->io_poller = io;
    io_curlm->multi = multi;
    io_curlm->timeout = -1;
    io_curlm->should_perform = true;
    curl_multi_setopt(multi, CURLMOPT_TIMERFUNCTION, curl_timer_cb);
    curl_multi_setopt(multi, CURLMOPT_TIMERDATA, io_curlm);
    curl_multi_setopt(multi, CURLMOPT_SOCKETFUNCTION, curl_socket_cb);
    curl_multi_setopt(multi, CURLMOPT_SOCKETDATA, io_curlm);

modify:
    io_curlm->cb = cb;
    io_curlm->user_data = user_data;
    return true;
}

bool
io_poller_curlm_del(struct io_poller *io, CURLM *multi)
{
    for (int i = 0; i < io->curlm_cnt; i++) {
        if (io->curlm[i]->multi == multi) {
            int *fds = io->curlm[i]->fds;
            int fds_cnt = io->curlm[i]->fds_cnt;
            for (int i = 0; i < fds_cnt; i++)
                io_poller_socket_del(io, fds[i]);
            free(fds);
            curl_multi_setopt(multi, CURLMOPT_TIMERFUNCTION, NULL);
            curl_multi_setopt(multi, CURLMOPT_TIMERDATA, NULL);
            curl_multi_setopt(multi, CURLMOPT_SOCKETFUNCTION, NULL);
            curl_multi_setopt(multi, CURLMOPT_SOCKETDATA, NULL);
            free(io->curlm[i]);
            memmove(&io->curlm[i], &io->curlm[i + 1],
                    (--io->curlm_cnt - i) * sizeof *io->curlm);
            return true;
        }
    }
    return false;
}

bool
io_poller_curlm_enable_perform(struct io_poller *io, CURLM *multi)
{
    for (int i = 0; i < io->curlm_cnt; i++)
        if (io->curlm[i]->multi == multi)
            return (io->curlm[i]->should_perform = true);
    return false;
}
