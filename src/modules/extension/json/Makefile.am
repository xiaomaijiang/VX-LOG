SUBDIRS	= yajl
noinst_LTLIBRARIES	= libjson.la
libjson_la_SOURCES	= json.c json.h
libjson_la_LIBADD	= $(top_srcdir)/src/modules/extension/json/yajl/libyajl.la
xm_json_LTLIBRARIES	= xm_json.la
xm_json_la_SOURCES	= xm_json.c xm_json.h xm_json_funcproc_cb.c $(xm_json_la_BUILTSOURCES)
xm_json_la_LDFLAGS	= -module -no-undefined -avoid-version
xm_jsondir		= $(NX_MODULEDIR)/extension
xm_json_la_LIBADD	= libjson.la $(LIBNX)
xm_json_la_BUILTSOURCES	= expr-xm_json-funcproc.c expr-xm_json-funcproc.h
expr-xm_json-funcproc.c: $(top_builddir)/src/utils/codegen.pl xm_json-api.xml
	-$(top_builddir)/src/utils/codegen.pl xm_json-api.xml
expr-xm_json-funcproc.h: $(top_builddir)/src/utils/codegen.pl xm_json-api.xml
	-$(top_builddir)/src/utils/codegen.pl xm_json-api.xml

EXTRA_DIST		= xm_json-api.xml $(xm_json_la_BUILTSOURCES)
MAINTAINERCLEANFILES	+= $(xm_json_la_BUILTSOURCES)
