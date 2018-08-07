noinst_LTLIBRARIES		= libkvp.la
xm_kvp_LTLIBRARIES		= xm_kvp.la
xm_kvp_la_SOURCES		= xm_kvp.c xm_kvp.h xm_kvp_funcproc_cb.c $(xm_kvp_la_BUILTSOURCES)
libkvp_la_SOURCES		= kvp.c kvp.h
xm_kvp_la_LDFLAGS		= -module -no-undefined -avoid-version
xm_kvp_la_LIBADD		= libkvp.la $(LIBNX)
xm_kvpdir			= $(NX_MODULEDIR)/extension
xm_kvp_la_BUILTSOURCES		= expr-xm_kvp-funcproc.c expr-xm_kvp-funcproc.h
expr-xm_kvp-funcproc.c: $(top_builddir)/src/utils/codegen.pl xm_kvp-api.xml
	-$(top_builddir)/src/utils/codegen.pl xm_kvp-api.xml
expr-xm_kvp-funcproc.h: $(top_builddir)/src/utils/codegen.pl xm_kvp-api.xml
	-$(top_builddir)/src/utils/codegen.pl xm_kvp-api.xml

EXTRA_DIST		= xm_kvp-api.xml $(xm_kvp_la_BUILTSOURCES)
MAINTAINERCLEANFILES	+= $(xm_kvp_la_BUILTSOURCES)
