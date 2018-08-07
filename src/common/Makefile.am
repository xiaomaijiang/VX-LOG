noinst_LTLIBRARIES	= libnx.la libnxssl.la
#libnx_la_LDFLAGS	= -no-undefined

#FIXME put common headers into _HEADERS
libnx_la_BUILTSOURCES	= expr-tokens.c expr-grammar.c expr-grammar.h expr-core-funcproc.c expr-core-funcproc.h
BUILT_SOURCES		= $(libnx_la_BUILTSOURCES)
libnx_la_SOURCES	= cfgfile.h config_cache.c config_cache.h confparser.c \
      date.c date.h dlist.h error_debug.c error_debug.h event.c event.h str.c str.h\
      logdata.c logdata.h logdata-serialize.c logqueue.c logqueue.h module.c module.h \
      route.h serialize.h types.h value.c value.h value-serialize.c resource.h resource.c \
      readerfuncs.c writerfuncs.c exception.c exception.h cexcept.h context.c context.h \
      expr-grammar.y expr-tokens.l expr.c expr.h expr-parser.c expr-parser.h \
      expr-core-funcproc.c expr-core-funcproc.h expr-core-funcproc-cb.c schedule.c schedule.h \
      statvar.h statvar.c backtrace.c backtrace.h alloc.c alloc.h atomic.h strptime.c
libnx_la_LIBADD		= $(PCRE_LIBS)
libnx_la_CFLAGS		= $(PCRE_CFLAGS)
libnxssl_la_SOURCES	= ssl.c ssl.h
libnxssl_la_LIBADD	= $(LIBSSL)

expr-core-funcproc.c: $(top_builddir)/src/utils/codegen.pl core-api.xml
	-$(top_builddir)/src/utils/codegen.pl core-api.xml
expr-core-funcproc.h: $(top_builddir)/src/utils/codegen.pl core-api.xml
	-$(top_builddir)/src/utils/codegen.pl core-api.xml

EXTRA_DIST		= core-api.xml $(libnx_la_BUILTSOURCES)
dist_pkgdata_DATA	= core-fields.xml

MAINTAINERCLEANFILES	+= $(libnx_la_BUILTSOURCES)
