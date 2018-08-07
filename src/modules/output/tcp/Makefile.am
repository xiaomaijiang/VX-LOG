om_tcp_LTLIBRARIES	= om_tcp.la
om_tcp_la_SOURCES	= om_tcp.c om_tcp.h om_tcp_funcproc_cb.c $(om_tcp_la_BUILTSOURCES)
om_tcp_la_LDFLAGS	= -module -no-undefined -avoid-version
om_tcp_la_LIBADD	= $(LIBNX)
om_tcpdir		= $(NX_MODULEDIR)/output

om_tcp_la_BUILTSOURCES = expr-om_tcp-funcproc.c expr-om_tcp-funcproc.h
expr-om_tcp-funcproc.c: $(top_builddir)/src/utils/codegen.pl om_tcp-api.xml
	-$(top_builddir)/src/utils/codegen.pl om_tcp-api.xml
expr-om_tcp-funcproc.h: $(top_builddir)/src/utils/codegen.pl om_tcp-api.xml
	-$(top_builddir)/src/utils/codegen.pl om_tcp-api.xml

EXTRA_DIST		= om_tcp-api.xml $(om_tcp_la_BUILTSOURCES)
MAINTAINERCLEANFILES	+= $(om_tcp_la_BUILTSOURCES)
