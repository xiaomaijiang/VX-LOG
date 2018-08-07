noinst_LTLIBRARIES	= libnxcore.la libnxlog.la
libnxcore_la_SOURCES	= ctx.c ctx.h core.c core.h
libnxlog_la_SOURCES	= nxlog.c nxlog.h router.c router.h modules.c modules.h job.h
libnxlog_la_LIBADD	= $(top_builddir)/src/common/libnx.la
bin_PROGRAMS		= nxlog
nxlog_SOURCES		= main-unix.c
CFLAGS			+= -DNX_PIDFILE=\"$(NX_PIDFILE)\" -DNX_MODULEDIR=\"$(NX_MODULEDIR)\" -DNX_CACHEDIR=\"$(NX_CACHEDIR)\" -DNX_CONFIGFILE=\"$(NX_CONFIGFILE)\"
#LDFLAGS			= -rdynamic
nxlog_LDADD		= $(top_builddir)/src/common/libnx.la libnxcore.la libnxlog.la $(LIBCAP)
libnxcore_la_LIBADD	= $(top_builddir)/src/common/libnx.la
