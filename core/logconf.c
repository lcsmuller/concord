#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h> /* strcasecmp() */
#include <stdarg.h>
#include <unistd.h> /* getpid() */

#include "logconf.h"
#include "cog-utils.h"

#define JSMN_STRICT
#define JSMN_HEADER
#include "jsmn.h"
#include "jsmn-find.h"

static int
_logconf_eval_level(char level[])
{
    if (0 == strcasecmp(level, "TRACE")) return LOG_TRACE;
    if (0 == strcasecmp(level, "DEBUG")) return LOG_DEBUG;
    if (0 == strcasecmp(level, "INFO")) return LOG_INFO;
    if (0 == strcasecmp(level, "WARN")) return LOG_WARN;
    if (0 == strcasecmp(level, "ERROR")) return LOG_ERROR;
    if (0 == strcasecmp(level, "FATAL")) return LOG_FATAL;
    ERR("Log level doesn't exist: %s", level);
    return 0; /* make compiler happy */
}

static void
_log_nocolor_cb(log_Event *ev)
{
    char buf[16];

    buf[strftime(buf, sizeof(buf), "%H:%M:%S", ev->time)] = '\0';

    fprintf(ev->udata, "%s %-5s %s:%d: ", buf, level_strings[ev->level],
            ev->file, ev->line);

    vfprintf(ev->udata, ev->fmt, ev->ap);
    fprintf(ev->udata, "\n");
    fflush(ev->udata);
}

static void
_log_color_cb(log_Event *ev)
{
    char buf[16];

    buf[strftime(buf, sizeof(buf), "%H:%M:%S", ev->time)] = '\0';

    fprintf(ev->udata, "%s %s%-5s\x1b[0m \x1b[90m%s:%d:\x1b[0m ", buf,
            level_colors[ev->level], level_strings[ev->level], ev->file,
            ev->line);

    vfprintf(ev->udata, ev->fmt, ev->ap);
    fprintf(ev->udata, "\n");
    fflush(ev->udata);
}

static void
_logconf_check_disabled(struct logconf *conf)
{
    int i;

    for (i = 0; i < conf->disable_modules.size; ++i)
        if (0 == strcmp(conf->id, conf->disable_modules.ids[i]))
            conf->is_disabled = true;
}

static void
_logconf_lock(struct logconf *conf)
{
    if (conf->L->lock) conf->L->lock(true, conf->L->udata);
}

static void
_logconf_unlock(struct logconf *conf)
{
    if (conf->L->lock) conf->L->lock(false, conf->L->udata);
}

void
logconf_http(struct logconf *conf,
             struct loginfo *p_info,
             char url[],
             struct logconf_szbuf header,
             struct logconf_szbuf body,
             char label_fmt[],
             ...)
{
    uint64_t tstamp_ms;
    int counter;

    if (!conf) return;

    tstamp_ms = cog_timestamp_ms();

    _logconf_lock(conf);
    counter = ++*conf->counter;
    _logconf_unlock(conf);

    if (conf->http && conf->http->f) {
        char timestr[64], label[512];
        va_list label_args;
        int len;

        va_start(label_args, label_fmt);
        len = vsnprintf(label, sizeof(label), label_fmt, label_args);
        ASSERT_S((size_t)len < sizeof(label), "Out of bounds write attempt");
        va_end(label_args);

        cog_unix_ms_to_iso8601(timestr, sizeof(timestr), tstamp_ms);

        fprintf(conf->http->f, "%s [%s] - %s - %s\n", label, conf->id, timestr,
                url);
        if (!conf->intercept_cb
            || !conf->intercept_cb(conf, &header, &body, label)) {
            fprintf(conf->http->f, "%.*s%s%.*s\n", (int)header.size,
                    header.start, header.size ? "\n" : "", (int)body.size,
                    body.start);
        }
        fprintf(conf->http->f, "@@@_%d_@@@\n", counter);
        fflush(conf->http->f);
    }

    if (p_info) {
        p_info->rcounter = counter;
        p_info->rtstamp_ms = tstamp_ms;
    }
}

void
logconf_setup(struct logconf *conf, const char id[], FILE *fp)
{
    jsmn_parser parser;
    jsmntok_t tokens[256];
    size_t fsize;
    char *file;
    int ret;

    struct {
        char level[16];
        char filename[1028];
        bool quiet, use_color, overwrite;
        struct {
            bool enable;
            char filename[1028];
        } http;
    } l = { 0 };

    memset(conf, 0, sizeof *conf);

    ret = snprintf(conf->id, sizeof(conf->id), "%s", id);
    ASSERT_S((size_t)ret < sizeof(conf->id), "Out of bounds write attempt");

    conf->pid = getpid();
    conf->counter = calloc(1, sizeof *conf->counter);
    conf->L = calloc(1, sizeof *conf->L);

    if (!fp) return;

    conf->logger = calloc(1, sizeof *conf->logger);
    conf->http = calloc(1, sizeof *conf->http);

    file = cog_load_whole_file_fp(fp, &fsize);

    /* populate logger settings with the 'config.json' file */
    jsmn_init(&parser);
    if (0 < jsmn_parse(&parser, file, fsize, tokens,
                       sizeof(tokens) / sizeof *tokens))
    {
        jsmnf_loader loader;
        jsmnf_pair pairs[256];

        jsmnf_init(&loader);
        if (0 < jsmnf_load(&loader, file, tokens, parser.toknext, pairs,
                           sizeof(pairs) / sizeof *pairs))
        {
            jsmnf_pair *f;

            if ((f = jsmnf_find(pairs, file, "logging", 7))) {
                jsmnf_pair *f1;

                if ((f1 = jsmnf_find(f, file, "level", 5)))
                    snprintf(l.level, sizeof(l.level), "%.*s", (int)f1->v.len,
                             file + f1->v.pos);
                if ((f1 = jsmnf_find(f, file, "filename", 8)))
                    snprintf(l.filename, sizeof(l.filename), "%.*s",
                             (int)f1->v.len, file + f1->v.pos);
                if ((f1 = jsmnf_find(f, file, "quiet", 5)))
                    l.quiet = ('t' == file[f1->v.pos]);
                if ((f1 = jsmnf_find(f, file, "use_color", 9)))
                    l.use_color = ('t' == file[f1->v.pos]);
                if ((f1 = jsmnf_find(f, file, "overwrite", 9)))
                    l.overwrite = ('t' == file[f1->v.pos]);
                if ((f1 = jsmnf_find(f, file, "http", 4))) {
                    jsmnf_pair *f2;

                    if ((f2 = jsmnf_find(f1, file, "enable", 6)))
                        l.http.enable = ('t' == file[f2->v.pos]);
                    if ((f2 = jsmnf_find(f1, file, "filename", 8)))
                        snprintf(l.http.filename, sizeof(l.http.filename),
                                 "%.*s", (int)f2->v.len, file + f2->v.pos);
                }
                if ((f1 = jsmnf_find(f, file, "disable_modules", 15))
                    && f1->size) {
                    int i = 0;

                    conf->disable_modules.ids =
                        malloc(f1->size * sizeof(char *));
                    for (i = 0; i < f1->size; ++i) {
                        jsmnf_pair *f2 = f1->fields + i;

                        if (f2->type == JSMN_STRING) {
                            const size_t length = f2->v.len + 1;
                            char *buf;

                            buf = malloc(length);
                            memcpy(buf, file + f2->v.pos, f2->v.len);
                            buf[f2->v.len] = '\0';

                            conf->disable_modules.ids[i] = buf;
                        }
                    }
                    conf->disable_modules.size = f1->size;
                }
            }
        }
    }

    conf->file.start = file;
    conf->file.size = fsize;

    /* skip everything else if this module is disabled */
    _logconf_check_disabled(conf);

    /* SET LOGGER CONFIGS */
    if (*l.filename) {
        conf->logger->fname = strdup(l.filename);
        conf->logger->f =
            fopen(conf->logger->fname, l.overwrite ? "w+" : "a+");
        ASSERT_S(NULL != conf->logger->f, "Could not create logger file");

        logconf_add_callback(conf,
                             l.use_color ? &_log_color_cb : &_log_nocolor_cb,
                             conf->logger->f, _logconf_eval_level(l.level));
    }

    /* SET HTTP DUMP CONFIGS */
    if (l.http.enable && *l.http.filename) {
        conf->http->fname = strdup(l.http.filename);
        conf->http->f = fopen(conf->http->fname, l.overwrite ? "w+" : "a+");
        ASSERT_S(NULL != conf->http->f, "Could not create http logger file");
    }

    /* disable default log.c callbacks */
    logconf_set_quiet(conf, true);

    /* make sure fatal still prints to stderr */
    logconf_add_callback(conf, l.use_color ? &_log_color_cb : &_log_nocolor_cb,
                         stderr,
                         l.quiet ? LOG_FATAL : _logconf_eval_level(l.level));
}

void
logconf_branch(struct logconf *branch, struct logconf *orig, const char id[])
{
    if (!orig) {
        logconf_setup(branch, id, NULL);
        return;
    }

    _logconf_lock(orig);
    memcpy(branch, orig, sizeof(struct logconf));
    _logconf_unlock(orig);

    branch->is_branch = true;
    if (id) {
        int len = snprintf(branch->id, sizeof(branch->id), "%s", id);
        ASSERT_S((size_t)len < sizeof(branch->id),
                 "Out of bounds write attempt");
    }
    branch->pid = getpid();

    _logconf_check_disabled(branch);
}

void
logconf_cleanup(struct logconf *conf)
{
    if (!conf->is_branch) {
        if (conf->file.start) {
            free(conf->file.start);
        }
        if (conf->logger) {
            if (conf->logger->fname) free(conf->logger->fname);
            if (conf->logger->f) fclose(conf->logger->f);
            free(conf->logger);
        }
        if (conf->http) {
            if (conf->http->fname) free(conf->http->fname);
            if (conf->http->f) fclose(conf->http->f);
            free(conf->http);
        }
        if (conf->disable_modules.ids) {
            int i;

            for (i = 0; i < conf->disable_modules.size; ++i)
                free(conf->disable_modules.ids[i]);
            free(conf->disable_modules.ids);
        }
        free(conf->counter);
        free(conf->L);
    }
    memset(conf, 0, sizeof *conf);
}

struct logconf_field
logconf_get_field(struct logconf *conf, char *const path[], unsigned depth)
{
    struct logconf_field field = { 0 };
    jsmn_parser parser;
    jsmntok_t tokens[256];

    if (!conf->file.size) return field;

    jsmn_init(&parser);
    if (0 < jsmn_parse(&parser, conf->file.start, conf->file.size, tokens,
                       sizeof(tokens) / sizeof *tokens))
    {
        jsmnf_loader loader;
        jsmnf_pair pairs[256];

        jsmnf_init(&loader);
        if (0 < jsmnf_load(&loader, conf->file.start, tokens, parser.toknext,
                           pairs, sizeof(pairs) / sizeof *pairs))
        {
            jsmnf_pair *f;
            if ((f = jsmnf_find_path(pairs, conf->file.start, path, depth))) {
                field.start = conf->file.start + f->v.pos;
                field.size = f->v.len;
            }
        }
    }
    return field;
}

void
logconf_set_lock(struct logconf *conf, log_LockFn fn, void *udata)
{
    _log_set_lock(conf->L, fn, udata);
}

void
logconf_set_level(struct logconf *conf, int level)
{
    _log_set_level(conf->L, level);
}

void
logconf_set_quiet(struct logconf *conf, bool enable)
{
    _log_set_quiet(conf->L, enable);
}

void
logconf_add_callback(struct logconf *conf,
                     log_LogFn fn,
                     void *udata,
                     int level)
{
    _log_add_callback(conf->L, fn, udata, level);
}

int
logconf_add_fp(struct logconf *conf, FILE *fp, int level)
{
    return _log_add_fp(conf->L, fp, level);
}
