om_http_LTLIBRARIES	= om_http.la
om_http_la_SOURCES	= om_http.c om_http.h om_http_funcproc_cb.c $(om_http_la_BUILTSOURCES)
om_http_la_LDFLAGS	= -module -no-undefined -avoid-version
om_http_la_LIBADD	= $(LIBNX) $(top_builddir)/src/common/libnxssl.la
om_httpdir		= $(NX_MODULEDIR)/output

om_http_la_BUILTSOURCES = expr-om_http-funcproc.c expr-om_http-funcproc.h
expr-om_http-funcproc.c: $(top_builddir)/src/utils/codegen.pl om_http-api.xml
	-$(top_builddir)/src/utils/codegen.pl om_http-api.xml
expr-om_http-funcproc.h: $(top_builddir)/src/utils/codegen.pl om_http-api.xml
	-$(top_builddir)/src/utils/codegen.pl om_http-api.xml

EXTRA_DIST		= om_http-api.xml $(om_http_la_BUILTSOURCES)
MAINTAINERCLEANFILES	+= $(om_http_la_BUILTSOURCES)
