## Process this file with automake to produce Makefile.in
##
    AUTOMAKE_OPTIONS = subdir-objects

         AM_CPPFLAGS = $(SPOA_MIRROR_CPPFLAGS)
           AM_CFLAGS = $(SPOA_MIRROR_CFLAGS)
          AM_LDFLAGS = $(SPOA_MIRROR_LDFLAGS)

decode_data_CPPFLAGS = $(AM_CPPFLAGS) -DPACKAGE_BUILD=`cat ../src/.build-counter` -I../include
  decode_data_CFLAGS = $(AM_CFLAGS)
 decode_data_LDFLAGS =
 decode_data_SOURCES = ../src/spoe-decode.c decode-data.c util.c

        bin_PROGRAMS = decode-data
          CLEANFILES = a.out

clean: clean-am
##
## Makefile.am ends here
