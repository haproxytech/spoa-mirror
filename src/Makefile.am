## Process this file with automake to produce Makefile.in
##
       AM_CPPFLAGS = $(SPOA_MIRROR_CPPFLAGS)
         AM_CFLAGS = $(SPOA_MIRROR_CFLAGS)
        AM_LDFLAGS = $(SPOA_MIRROR_LDFLAGS)
              LIBS = @LIBS@ @SPOA_MIRROR_LIBS@

spoa_mirror_CPPFLAGS = $(AM_CPPFLAGS) -DPACKAGE_BUILD=`cat .build-counter` -I../include
  spoa_mirror_CFLAGS = $(AM_CFLAGS)
 spoa_mirror_LDFLAGS = $(AM_LDFLAGS)
        bin_PROGRAMS = spoa-mirror
 spoa_mirror_SOURCES = \
	main.c \
	spoa-message.c \
	spoa.c \
	spoe-decode.c \
	spoe-encode.c \
	spoe.c \
	spop-ack.c \
	spop-disconnect.c \
	spop-hello.c \
	spop-notify.c \
	spop-unset.c \
	tcp.c \
	util.c \
	worker.c

if WANT_CURL
spoa_mirror_SOURCES += curl.c
endif

CLEANFILES = a.out

clean: clean-am build-counter

build-counter:
	$(AM_V_GEN)../scripts/build-counter
##
## Makefile.am ends here
