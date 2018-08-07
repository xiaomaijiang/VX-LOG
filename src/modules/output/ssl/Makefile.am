om_ssl_LTLIBRARIES	= om_ssl.la
om_ssl_la_SOURCES	= om_ssl.c om_ssl.h om_ssl_funcproc_cb.c $(om_ssl_la_BUILTSOURCES)
om_ssl_la_LDFLAGS	= -module -no-undefined -avoid-version
om_ssl_la_LIBADD	= $(LIBNX) $(top_builddir)/src/common/libnxssl.la
om_ssldir		= $(NX_MODULEDIR)/output

om_ssl_la_BUILTSOURCES = expr-om_ssl-funcproc.c expr-om_ssl-funcproc.h
expr-om_ssl-funcproc.c: $(top_builddir)/src/utils/codegen.pl om_ssl-api.xml
	-$(top_builddir)/src/utils/codegen.pl om_ssl-api.xml
expr-om_ssl-funcproc.h: $(top_builddir)/src/utils/codegen.pl om_ssl-api.xml
	-$(top_builddir)/src/utils/codegen.pl om_ssl-api.xml

EXTRA_DIST		= om_ssl-api.xml $(om_ssl_la_BUILTSOURCES)
MAINTAINERCLEANFILES	+= $(om_ssl_la_BUILTSOURCES)
