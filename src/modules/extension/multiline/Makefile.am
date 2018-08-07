xm_multiline_LTLIBRARIES	= xm_multiline.la
#xm_multiline_la_BUILTSOURCES	= expr-xm_multiline-funcproc.c expr-xm_multiline-funcproc.h
#xm_multiline_la_SOURCES		= xm_multiline.c xm_multiline.h xm_multiline_funcproc_cb.c $(xm_multiline_la_BUILTSOURCES)
xm_multiline_la_SOURCES		= xm_multiline.c xm_multiline.h
xm_multiline_la_LDFLAGS		= -module -no-undefined -avoid-version
xm_multiline_la_LIBADD		= $(LIBNX) $(PCRE_LIBS)
xm_multiline_la_CFLAGS		= $(PCRE_CFLAGS)
xm_multilinedir			= $(NX_MODULEDIR)/extension

#expr-xm_multiline-funcproc.c: $(top_builddir)/src/utils/codegen.pl xm_multiline-api.xml
#	-$(top_builddir)/src/utils/codegen.pl xm_multiline-api.xml
#expr-xm_multiline-funcproc.h: $(top_builddir)/src/utils/codegen.pl xm_multiline-api.xml
#	-$(top_builddir)/src/utils/codegen.pl xm_multiline-api.xml

#EXTRA_DIST		= xm_multiline-api.xml $(xm_multiline_la_BUILTSOURCES)
#MAINTAINERCLEANFILES	+= $(xm_multiline_la_BUILTSOURCES)
