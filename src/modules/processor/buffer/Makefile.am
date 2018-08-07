pm_buffer_LTLIBRARIES	= pm_buffer.la
pm_buffer_la_SOURCES	= pm_buffer.c pm_buffer.h pm_buffer_funcproc_cb.c $(pm_buffer_la_BUILTSOURCES)
pm_buffer_la_LDFLAGS	= -module -no-undefined -avoid-version
pm_buffer_la_LIBADD	= $(LIBNX)
pm_bufferdir		= $(NX_MODULEDIR)/processor
pm_buffer_la_BUILTSOURCES = expr-pm_buffer-funcproc.c expr-pm_buffer-funcproc.h

expr-pm_buffer-funcproc.c: $(top_builddir)/src/utils/codegen.pl pm_buffer-api.xml
	-$(top_builddir)/src/utils/codegen.pl pm_buffer-api.xml
expr-pm_buffer-funcproc.h: $(top_builddir)/src/utils/codegen.pl pm_buffer-api.xml
	-$(top_builddir)/src/utils/codegen.pl pm_buffer-api.xml

EXTRA_DIST		= pm_buffer-api.xml $(pm_buffer_la_BUILTSOURCES)
MAINTAINERCLEANFILES	+= $(pm_buffer_la_BUILTSOURCES)
