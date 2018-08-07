noinst_LTLIBRARIES	= libsyslog.la
libsyslog_la_SOURCES	= syslog.c syslog.h

xm_syslog_LTLIBRARIES	= xm_syslog.la
xm_syslog_la_BUILTSOURCES = expr-xm_syslog-funcproc.c expr-xm_syslog-funcproc.h
xm_syslog_la_SOURCES	= xm_syslog.c xm_syslog.h xm_syslog_funcproc_cb.c $(xm_syslog_la_BUILTSOURCES)
xm_syslog_la_LDFLAGS	= -module -no-undefined -avoid-version
xm_syslog_la_LIBADD	= libsyslog.la $(LIBNX)
xm_syslogdir		= $(NX_MODULEDIR)/extension

expr-xm_syslog-funcproc.c: $(top_builddir)/src/utils/codegen.pl xm_syslog-api.xml
	-$(top_builddir)/src/utils/codegen.pl xm_syslog-api.xml
expr-xm_syslog-funcproc.h: $(top_builddir)/src/utils/codegen.pl xm_syslog-api.xml
	-$(top_builddir)/src/utils/codegen.pl xm_syslog-api.xml

EXTRA_DIST		= xm_syslog-api.xml $(xm_syslog_la_BUILTSOURCES)
dist_pkgdata_DATA	= xm_syslog-fields.xml
MAINTAINERCLEANFILES	+= $(xm_syslog_la_BUILTSOURCES)
