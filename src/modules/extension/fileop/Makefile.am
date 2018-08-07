xm_fileop_LTLIBRARIES		= xm_fileop.la
xm_fileop_la_SOURCES		= xm_fileop.c xm_fileop_funcproc_cb.c $(xm_fileop_la_BUILTSOURCES)
xm_fileop_la_LDFLAGS		= -module -no-undefined -avoid-version
xm_fileop_la_LIBADD		= $(LIBNX)
xm_fileopdir			= $(NX_MODULEDIR)/extension
xm_fileop_la_BUILTSOURCES	= expr-xm_fileop-funcproc.c expr-xm_fileop-funcproc.h
expr-xm_fileop-funcproc.c: $(top_builddir)/src/utils/codegen.pl xm_fileop-api.xml
	-$(top_builddir)/src/utils/codegen.pl xm_fileop-api.xml
expr-xm_fileop-funcproc.h: $(top_builddir)/src/utils/codegen.pl xm_fileop-api.xml
	-$(top_builddir)/src/utils/codegen.pl xm_fileop-api.xml

EXTRA_DIST			= xm_fileop-api.xml $(xm_fileop_la_BUILTSOURCES)
MAINTAINERCLEANFILES    	+= $(xm_fileop_la_BUILTSOURCES)
