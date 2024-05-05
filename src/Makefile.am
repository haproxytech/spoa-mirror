## Process this file with automake to produce Makefile.in
##
   AM_CPPFLAGS = @SPOA_MIRROR_CPPFLAGS@ -I$(top_builddir)/include -I$(srcdir)/../include -DPACKAGE_BUILD=`cat $(srcdir)/../src/.build-counter`
     AM_CFLAGS = @SPOA_MIRROR_CFLAGS@
    AM_LDFLAGS = @SPOA_MIRROR_LDFLAGS@
          LIBS = @LIBS@ @SPOA_MIRROR_LIBS@

common_SOURCES = \
	libev.c \
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
common_SOURCES += curl.c
endif

if WANT_DEBUG
           bin_PROGRAMS = spoa-mirror_dbg
spoa_mirror_dbg_SOURCES = $(common_SOURCES)

else

           bin_PROGRAMS = spoa-mirror
    spoa_mirror_SOURCES = $(common_SOURCES)
endif

CLEANFILES = a.out

clean: clean-am build-counter

build-counter:
	$(AM_V_GEN)../scripts/build-counter
##
## Makefile.am ends here
