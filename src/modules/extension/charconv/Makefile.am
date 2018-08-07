if HAVE_ICONV
xm_charconv_LTLIBRARIES		= xm_charconv.la
xm_charconv_la_SOURCES		= charconv.c charconv.h xm_charconv.c xm_charconv.h xm_charconv_funcproc_cb.c $(xm_charconv_la_BUILTSOURCES)
xm_charconv_la_LDFLAGS		= -module -no-undefined -avoid-version
xm_charconv_la_LIBADD		= $(LIBNX) $(LIBICONV)
xm_charconvdir			= $(NX_MODULEDIR)/extension
xm_charconv_la_BUILTSOURCES	= expr-xm_charconv-funcproc.c expr-xm_charconv-funcproc.h
endif

expr-xm_charconv-funcproc.c: $(top_builddir)/src/utils/codegen.pl xm_charconv-api.xml
	-$(top_builddir)/src/utils/codegen.pl xm_charconv-api.xml
expr-xm_charconv-funcproc.h: $(top_builddir)/src/utils/codegen.pl xm_charconv-api.xml
	-$(top_builddir)/src/utils/codegen.pl xm_charconv-api.xml

EXTRA_DIST			= xm_charconv-api.xml $(xm_charconv_la_BUILTSOURCES)
MAINTAINERCLEANFILES    	+= $(xm_charconv_la_BUILTSOURCES)
