
CC	=	m68k-amigaos-gcc
CFLAGS	=	-noixemul -m68020 -O2 -fomit-frame-pointer -g -I. -W -Wall

OBJS	=	daemons.c daemons_bind.c daemons_loop.c daemons_progname.c daemons_resolve.c daemons_services.c daemons_signals.c daemons_syslog_debug.c daemons_utils.c malloc.c malloc_memflags.c malloc_minsize.c malloc_puddleSize.c malloc_threshSize.c daemons_recvln_timeout.c daemons_recv_timeout.c daemons_recvsend.c daemons_forks.c daemons_fork.c daemons_signal_dispatcher.c daemons_forkerror.c daemons_bindretard.c daemons_child_priority.c daemons_child_stacksize.c daemons_child_xtratags.c daemons_clock.c

all:	$(OBJS:.c=.o)
	m68k-amigaos-ar cru libdaemons.a $(OBJS:.c=.o)
	m68k-amigaos-ranlib libdaemons.a

test:
	$(CC) $(CFLAGS) dtest.c -o dtest -L. -ldaemons -ldebug -Wl,-Map,dtest.map,--cref

install:
	cp -p libdaemons.a /gg/m68k-amigaos/lib/libdaemons.a
	m68k-amigaos-ranlib /gg/m68k-amigaos/lib/libdaemons.a
	cp -p ./daemons_public.h /gg/include/daemons.h

clean:
	rm -f *.o

