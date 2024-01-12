#ifndef LOGCONF_H
#define LOGCONF_H

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdint.h> /* uint64_t */

#include "attributes.h"
#include "log.h"

#define __ERR(fmt, ...) log_fatal(fmt "%s", __VA_ARGS__)

/**
 * @brief Print error message and abort
 *
 * @param ... printf-like `format` and variadic arguments (if any)
 */
#define ERR(...)                                                              \
    do {                                                                      \
        __ERR(__VA_ARGS__, "");                                               \
        abort();                                                              \
    } while (0)

/**
 * @brief Assert that allows printing a error message
 *
 * @param expr conditional expression that's expected to be true
 * @param msg error message
 */
#define ASSERT_S(expr, msg)                                                   \
    do {                                                                      \
        if (!(expr)) {                                                        \
            ERR(ANSICOLOR(                                                    \
                    "\n\tAssert Failed",                                      \
                    ANSI_FG_RED) ":\t%s\n\t" ANSICOLOR("Expected",            \
                                                       ANSI_FG_RED) ":"       \
                                                                    "\t" msg, \
                #expr);                                                       \
        }                                                                     \
    } while (0)

/**
 * @brief Assert that allows printing a error message in a printf-like fashion
 * @warning if no variadic arguments are specified there will be errors, in
 *      that case use @ref ASSERT_S.
 *
 * @param expr conditional expression that's expected to be true
 * @param fmt printf-like formatting string for the error message
 * @param ... printf-like variadic arguments to be matched to `fmt`
 */
#define VASSERT_S(expr, fmt, ...)                                             \
    do {                                                                      \
        if (!(expr)) {                                                        \
            ERR(ANSICOLOR("\n\tAssert Failed",                                \
                          ANSI_FG_RED) ":\t" fmt                              \
                                       "\n\t" ANSICOLOR(                      \
                                           "Expected", ANSI_FG_RED) ":\t %s", \
                __VA_ARGS__, #expr);                                          \
        }                                                                     \
    } while (0)

/* Encode a string with ANSI color */
#ifdef LOG_USE_COLOR
#define ANSICOLOR(str, color) "\x1b[" color "m" str "\x1b[0m"
#else
#define ANSICOLOR(str, color) str
#endif

#define ANSI_FG_BLACK          "30"
#define ANSI_FG_RED            "31"
#define ANSI_FG_GREEN          "32"
#define ANSI_FG_YELLOW         "33"
#define ANSI_FG_BLUE           "34"
#define ANSI_FG_MAGENTA        "35"
#define ANSI_FG_CYAN           "36"
#define ANSI_FG_WHITE          "37"
#define ANSI_FG_GRAY           "90"
#define ANSI_FG_BRIGHT_RED     "91"
#define ANSI_FG_BRIGHT_GREEN   "92"
#define ANSI_FG_BRIGHT_YELLOW  "93"
#define ANSI_FG_BRIGHT_BLUE    "94"
#define ANSI_FG_BRIGHT_MAGENTA "95"
#define ANSI_FG_BRIGHT_CYAN    "96"
#define ANSI_FG_BRIGHT_WHITE   "97"

#define ANSI_BG_BLACK          "40"
#define ANSI_BG_RED            "41"
#define ANSI_BG_GREEN          "42"
#define ANSI_BG_YELLOW         "43"
#define ANSI_BG_BLUE           "44"
#define ANSI_BG_MAGENTA        "45"
#define ANSI_BG_CYAN           "46"
#define ANSI_BG_WHITE          "47"
#define ANSI_BG_GRAY           "100"
#define ANSI_BG_BRIGHT_RED     "101"
#define ANSI_BG_BRIGHT_GREEN   "102"
#define ANSI_BG_BRIGHT_YELLOW  "103"
#define ANSI_BG_BRIGHT_BLUE    "104"
#define ANSI_BG_BRIGHT_MAGENTA "105"
#define ANSI_BG_BRIGHT_CYAN    "106"
#define ANSI_BG_BRIGHT_WHITE   "107"

/** @defgroup Log_C_Datatypes
 * @brief Relevant datatypes borrowed from `log.c`
 * @see Read `log.c`
 *  <a href="https://github.com/rxi/log.c#usage">documentation</a>
 * @{ */
/**
 * @var log_Level
 * @struct log_Logger
 * @struct log_Callback
 * @struct log_Event
 * @typedef log_LockFn
 * @typedef log_LogFn
 * @def LOG_MAX_CALLBACKS
 */
/** @} */

/**
 * @brief Log level trace
 *
 * @param conf the `struct logconf` module
 * @param ... the printf-like format string and successive arguments
 */
#define logconf_trace(conf, ...)                                              \
    logconf_log(conf, LOG_TRACE, __FILE__, __LINE__, __VA_ARGS__)
/**
 * @brief Log level debug
 *
 * @param conf the `struct logconf` module
 * @param ... the printf-like format string and successive arguments
 */
#define logconf_debug(conf, ...)                                              \
    logconf_log(conf, LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)
/**
 * @brief Log level info
 *
 * @param conf the `struct logconf` module
 * @param ... the printf-like format string and successive arguments
 */
#define logconf_info(conf, ...)                                               \
    logconf_log(conf, LOG_INFO, __FILE__, __LINE__, __VA_ARGS__)
/**
 * @brief Log level warn
 *
 * @param conf the `struct logconf` module
 * @param ... the printf-like format string and successive arguments
 */
#define logconf_warn(conf, ...)                                               \
    logconf_log(conf, LOG_WARN, __FILE__, __LINE__, __VA_ARGS__)
/**
 * @brief Log level error
 *
 * @param conf the `struct logconf` module
 * @param ... the printf-like format string and successive arguments
 */
#define logconf_error(conf, ...)                                              \
    logconf_log(conf, LOG_ERROR, __FILE__, __LINE__, __VA_ARGS__)
/**
 * @brief Log level fatal
 *
 * @param conf the `struct logconf` module
 * @param ... the printf-like format string and successive arguments
 */
#define logconf_fatal(conf, ...)                                              \
    logconf_log(conf, LOG_FATAL, __FILE__, __LINE__, __VA_ARGS__)

/* helper function for logconf_log() */
#define __logconf_log(conf, level, file, line, fmt, ...)                      \
    _log_log((conf)->L, level, file, line, "[%s] " fmt "%s", (conf)->id,      \
             __VA_ARGS__)
/**
 * @brief Run-time configurable log level
 *
 * @param conf the `struct logconf` module
 * @param level the log level enumerator from `log.c`
 * @param file the origin file name
 * @param line the origin file line
 * @param ... the printf-like format string and successive arguments
 */
#define logconf_log(conf, level, file, line, ...)                             \
    if (!(conf)->is_disabled)                                                 \
    __logconf_log(conf, level, file, line, __VA_ARGS__, "")

/** Maximum length for module id */
#define LOGCONF_ID_LEN 64 + 1

/**
 * @brief The read-only `config.json` field
 * @see logconf_get_field()
 */
struct logconf_field {
    /** the buffer's start */
    const char *start;
    /** the buffer's size in bytes */
    size_t size;
};

/** @brief Generic sized-buffer */
struct logconf_szbuf {
    /** the buffer's start */
    char *start;
    /** the buffer's size in bytes */
    size_t size;
};

struct logconf;

/** @brief Intercept logging entries for logging customization and filtering
 *      purposes */
typedef _Bool (*logconf_intercept_cb)(const struct logconf *conf,
                                      const struct logconf_szbuf *header,
                                      const struct logconf_szbuf *body,
                                      const char label[]);

/**
 * @brief A stackful and modularized wrapper over the popular 'log.c'
 * facilities
 *
 * Provides a way to split logging into individual modules, that can be
 * configured separately from one another, or have their resources shared via
 * branching.
 */
struct logconf {
    /** logging module id */
    char id[LOGCONF_ID_LEN];
    /** the id of the process where this module was created */
    unsigned pid;
    /** if true then logconf_cleanup() won't cleanup shared resources */
    _Bool is_branch;
    /** if true then logging will be ignored for this module */
    _Bool is_disabled;
    /** config file contents */
    struct logconf_szbuf file;

    /** http logging counter */
    int *counter;
    /** log.c main structure (shared with branches) */
    log_Logger *L;

    /** intercept log entry callback */
    logconf_intercept_cb intercept_cb;

    struct {
        /** name of logging output file */
        char *fname;
        /** pointer to logging output file */
        FILE *f;
    } * logger, *http;

    /** list of 'id' that should be ignored */
    struct {
        size_t size;
        char **ids;
    } disable_modules;
};

/** @brief Store logging information from logconf_http() */
struct loginfo {
    /** readable log count */
    size_t rcounter;
    /** readable log timestamp */
    uint64_t rtstamp_ms;
};

/**
 * @brief Initialize a `struct logconf` module from a config file
 *
 * @param conf pointer to the `struct logconf` structure to be initialized
 * @param id the `struct logconf` module id
 * @param fp the configuration file pointer that will fill `struct logconf`
 * fields
 * @see logconf_get_field() for fetching config file field's value
 */
void logconf_setup(struct logconf *conf, const char id[], FILE *fp);

/**
 * @brief Branch and link a `struct logconf` module to a existing one
 *
 * Initialize a `branch` logging module thats expected to share common
 * resources with its parent module `orig`. The common resources
 * include: config file directives, logging output and disabled modules list.
 * @param branch pointer to the `struct logconf` structure to be initialized as
 * `orig` branch
 * @param orig pointer to the `struct logconf` structure that holds the parent
 * module
 * @param id the `branch` module id
 */
void logconf_branch(struct logconf *branch,
                    struct logconf *orig,
                    const char id[]);

/**
 * @brief Cleanup a `struct logconf` module
 *
 * @param conf the `struct logconf` structure to be cleaned
 */
void logconf_cleanup(struct logconf *conf);

/**
 * @brief Get the value from a given JSON field of the config file
 *
 * @param conf the `struct logconf` module
 * @param path the JSON key path
 * @param depth the path depth
 * @return a read-only sized buffer containing the field's contents
 * @see logconf_setup() for initializing `conf` with a config file
 */
struct logconf_field logconf_get_field(struct logconf *conf,
                                       char *const path[],
                                       unsigned depth);

/**
 * @brief Log HTTP transfers
 *
 * @param conf the `struct logconf` module
 * @param info retrieve information on this logging
 * @param url the transfer URL
 * @param header the transfer header
 * @param body the transfer body
 * @param label_fmt a `printf()` like formatting string
 *        to provide additional logging description, such as the
 *        transfer's HTTP method, or HTTP response code.
 * @param ... subsequent arguments that are converted for output
 */
void logconf_http(struct logconf *conf,
                  struct loginfo *info,
                  char url[],
                  struct logconf_szbuf header,
                  struct logconf_szbuf body,
                  char label_fmt[],
                  ...) PRINTF_LIKE(6, 7);

/**
 * @brief If the log will be written to from multiple threads a lock function
 * can be set.
 *
 * The function is passed the boolean true if the lock should be acquired or
 * false if the lock should be released and the given udata value.
 * @param conf the `struct logconf` module
 * @param fn lock callback
 * @param udata user arbitrary data
 */
void logconf_set_lock(struct logconf *conf, log_LockFn fn, void *udata);

/**
 * @brief Set the current logging level
 *
 * All logs below the given level will not be written to stderr. By default the
 * level is LOG_TRACE, such that nothing is ignored.
 * @param conf the `struct logconf` module
 * @param level logging level
 */
void logconf_set_level(struct logconf *conf, int level);

/**
 * @brief Toggle quiet mode
 *
 * Quiet-mode can be enabled by settings `enable` to `true`. While this mode is
 * enabled the library will not output anything to stderr, but will continue to
 * write to files and callbacks if any are set.
 * @param conf the `struct logconf` module
 * @param enable `true` enables quiet-mode
 */
void logconf_set_quiet(struct logconf *conf, bool enable);

/**
 * @brief Callback functions called when logging data
 *
 * One or more callback functions which are called with the log data can be
 * provided to the library. A callback function is passed a log_Event structure
 * containing the line number, filename, fmt string, va printf va_list, level
 * and the given udata.
 * @param conf the `struct logconf` module
 * @param fn the callback function
 * @param udata user arbitrary data
 * @param level logging level to trigger callback
 */

void logconf_add_callback(struct logconf *conf,
                          log_LogFn fn,
                          void *udata,
                          int level);

/**
 * @brief File where the log will be written
 *
 * One or more file pointers where the log will be written can be provided to
 * the library. Any messages below the given `level` are ignored. If the
 * library failed to add a file pointer a value less-than-zero is returned.
 * @param conf the `struct logconf` module
 * @param fp the write-to file pointer
 * @param level logging level condition for writing to `fp`
 */
int logconf_add_fp(struct logconf *conf, FILE *fp, int level);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LOGCONF_H */
