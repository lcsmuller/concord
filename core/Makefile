PREFIX = /usr/local

OBJS = cog-utils.o      \
       io_poller.o      \
       user-agent.o     \
       websockets.o     \
       jsmn-find.o      \
       json-build.o     \
       log.o            \
       logconf.o        \
       priority_queue.o \
       anomap.o         \
       threadpool.o     \
       queriec.o

WFLAGS  = -Wall -Wextra -Wpedantic
CFLAGS += -std=c99 -pthread -D_XOPEN_SOURCE=600 -DLOG_USE_COLOR \
          -I. -I$(PREFIX)/include # $(WFLAGS)

all: $(OBJS)

echo:
	@ echo -e 'CC: $(CC)\n'
	@ echo -e 'CFLAGS: $(CFLAGS)\n'
	@ echo -e 'OBJS: $(OBJS)\n'

clean: 
	@ rm -f $(OBJS)

.PHONY: echo clean
