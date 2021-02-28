## LIBOPTS Makefile
MAINTAINERCLEANFILES    = Makefile.in
if INSTALL_LIBOPTS
lib_LTLIBRARIES         = libopts.la
else
noinst_LTLIBRARIES      = libopts.la
endif
libopts_la_SOURCES      = libopts.c
libopts_la_CPPFLAGS     = -I$(top_srcdir)
libopts_la_LDFLAGS      = -version-info  33:2:8
EXTRA_DIST              = \
    ag-char-map.h           autoopts/options.h      autoopts/usage-txt.h  \
    autoopts.c              autoopts.h              boolean.c  \
    compat/compat.h         compat/strdup.c         compat/windows-config.h  \
    compat/pathfind.c       compat/snprintf.c       compat/strchr.c  \
    configfile.c            cook.c                  COPYING.gplv3  \
    COPYING.lgplv3          COPYING.mbsd            enumeration.c  \
    environment.c           file.c                  genshell.c  \
    genshell.h              load.c                  m4/liboptschk.m4  \
    m4/libopts.m4           MakeDefs.inc            makeshell.c  \
    nested.c                numeric.c               parse-duration.c  \
    parse-duration.h        pgusage.c               proto.h  \
    putshell.c              README                  reset.c  \
    restore.c               save.c                  sort.c  \
    stack.c                 streqvcmp.c             text_mmap.c  \
    time.c                  tokenize.c              usage.c  \
    value-type.c            value-type.h            version.c  \
    xat-attribute.c         xat-attribute.h
