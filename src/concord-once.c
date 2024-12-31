#include <string.h>
#include <signal.h>
#include <curl/curl.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/ioctl.h>

#include "error.h"
#include "discord-worker.h"

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

static int shutdown_fds[2] = {
    -1,
    -1,
};

static int init_counter = 0;

void
ccord_shutdown_async(void)
{
    char b = 0;
    write(shutdown_fds[1], &b, sizeof b);
}

int
ccord_shutting_down(void)
{
    struct pollfd pfd = {
        .fd = shutdown_fds[0],
        .events = POLLIN,
    };
    if (-1 == shutdown_fds[0]) return 0;
    poll(&pfd, 1, 0);
    return !!(pfd.revents & POLLIN);
}

#ifdef CCORD_SIGINTCATCH
/* shutdown gracefully on SIGINT received */
static void
_ccord_sigint_handler(int signum)
{
    (void)signum;
    const char err_str[] =
        "\nSIGINT: Disconnecting running concord client(s) ...\n";
    write(STDERR_FILENO, err_str, strlen(err_str));
    ccord_shutdown_async();
}
#endif /* CCORD_SIGINTCATCH */

CCORDcode
ccord_global_init()
{
    pthread_mutex_lock(&lock);
    if (0 == init_counter++) {
#ifdef CCORD_SIGINTCATCH
        signal(SIGINT, &_ccord_sigint_handler);
#endif
        if (0 != curl_global_init(CURL_GLOBAL_DEFAULT)) {
            fputs("Couldn't start libcurl's globals\n", stderr);
            goto fail_curl_init;
        }
        if (0 != discord_worker_global_init()) {
            fputs("Attempt duplicate global initialization\n", stderr);
            goto fail_discord_worker_init;
        }
        if (0 != pipe(shutdown_fds)) {
            fputs("Failed to create shutdown pipe\n", stderr);
            goto fail_pipe_init;
        }
        for (int i = 0; i < 2; i++) {
#ifdef FIOCLEX
            if (0 != ioctl(shutdown_fds[i], FIOCLEX, NULL)) {
                fputs("Failed to make shutdown pipe close on execute\n",
                      stderr);
                goto fail_pipe_flags;
            }
#endif
            int flags = fcntl(shutdown_fds[i], F_GETFL) | O_NONBLOCK;
            if (0 != fcntl(shutdown_fds[i], F_SETFL, flags)) {
                fputs("Failed to make shutdown pipe nonblocking\n", stderr);
                goto fail_pipe_flags;
            }
        }
    }
    pthread_mutex_unlock(&lock);
    return CCORD_OK;

fail_pipe_flags:
    for (int i = 0; i < 2; i++) {
        if (-1 != shutdown_fds[i]) {
            close(shutdown_fds[i]);
            shutdown_fds[i] = -1;
        }
    }
fail_pipe_init:
    discord_worker_global_cleanup();
fail_discord_worker_init:
    curl_global_cleanup();
fail_curl_init:

    init_counter = 0;
    pthread_mutex_unlock(&lock);
    return CCORD_GLOBAL_INIT;
}

void
ccord_global_cleanup()
{
    pthread_mutex_lock(&lock);
    if (init_counter && 0 == --init_counter) {
        curl_global_cleanup();
        discord_worker_global_cleanup();
        for (int i = 0; i < 2; i++) {
            close(shutdown_fds[i]);
            shutdown_fds[i] = -1;
        }
    }
    pthread_mutex_unlock(&lock);
}

int
discord_dup_shutdown_fd(void)
{
    int fd = -1;
    if (-1 == shutdown_fds[0]) return -1;
    if (-1 != (fd = dup(shutdown_fds[0]))) {
#ifdef FIOCLEX
        if (0 != ioctl(fd, FIOCLEX, NULL)) {
            close(fd);
            return -1;
        }
#endif
        int flags = fcntl(fd, F_GETFL) | O_NONBLOCK;
        if (0 != fcntl(fd, F_SETFL, flags)) {
            close(fd);
            return -1;
        }
    }
    return fd;
}
