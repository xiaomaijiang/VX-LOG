xm_exec_LTLIBRARIES	= xm_exec.la
xm_exec_la_BUILTSOURCES = expr-xm_exec-funcproc.c expr-xm_exec-funcproc.h
xm_exec_la_SOURCES	= xm_exec.c xm_exec.h xm_exec_funcproc_cb.c $(xm_exec_la_BUILTSOURCES) xm_exec-api.xml
xm_exec_la_LDFLAGS	= -module -no-undefined -avoid-version
xm_exec_la_LIBADD	= $(LIBNX)
xm_execdir		= $(NX_MODULEDIR)/extension

expr-xm_exec-funcproc.c: $(top_builddir)/src/utils/codegen.pl xm_exec-api.xml
	-$(top_builddir)/src/utils/codegen.pl xm_exec-api.xml
expr-xm_exec-funcproc.h: $(top_builddir)/src/utils/codegen.pl xm_exec-api.xml
	-$(top_builddir)/src/utils/codegen.pl xm_exec-api.xml

EXTRA_DIST		= xm_exec-api.xml $(xm_exec_la_BUILTSOURCES)
MAINTAINERCLEANFILES	+= $(xm_exec_la_BUILTSOURCES)
