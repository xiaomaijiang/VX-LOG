pm_blocker_LTLIBRARIES	= pm_blocker.la
pm_blocker_la_SOURCES	= pm_blocker.c pm_blocker.h pm_blocker_funcproc_cb.c $(pm_blocker_la_BUILTSOURCES)
pm_blocker_la_LDFLAGS	= -module -no-undefined -avoid-version
pm_blocker_la_LIBADD	= $(LIBNX)
pm_blockerdir		= $(NX_MODULEDIR)/processor

pm_blocker_la_BUILTSOURCES = expr-pm_blocker-funcproc.c expr-pm_blocker-funcproc.h
expr-pm_blocker-funcproc.c: $(top_builddir)/src/utils/codegen.pl pm_blocker-api.xml
	-$(top_builddir)/src/utils/codegen.pl pm_blocker-api.xml
expr-pm_blocker-funcproc.h: $(top_builddir)/src/utils/codegen.pl pm_blocker-api.xml
	-$(top_builddir)/src/utils/codegen.pl pm_blocker-api.xml

EXTRA_DIST		= pm_blocker-api.xml $(pm_blocker_la_BUILTSOURCES)
MAINTAINERCLEANFILES	+= $(pm_blocker_la_BUILTSOURCES)
