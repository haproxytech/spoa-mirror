## Process this file with automake to produce Makefile.in
##
       AUTOMAKE_OPTIONS = subdir-objects

            AM_CPPFLAGS = @SPOA_MIRROR_CPPFLAGS@ -I$(top_builddir)/include -I$(srcdir)/../include -DPACKAGE_BUILD=`cat $(srcdir)/../src/.build-counter`
              AM_CFLAGS = @SPOA_MIRROR_CFLAGS@
             AM_LDFLAGS = @SPOA_MIRROR_LDFLAGS@

         common_SOURCES = ../src/spoe-decode.c ../src/str.c decode-data.c util.c

if WANT_DEBUG
decode_data_dbg_LDFLAGS =
decode_data_dbg_SOURCES = $(common_SOURCES)

           bin_PROGRAMS = decode-data_dbg

else

    decode_data_LDFLAGS =
    decode_data_SOURCES = $(common_SOURCES)

           bin_PROGRAMS = decode-data
endif

CLEANFILES = a.out

clean: clean-am
##
## Makefile.am ends here
