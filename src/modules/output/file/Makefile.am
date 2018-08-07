om_file_LTLIBRARIES	= om_file.la
om_file_la_SOURCES	= om_file.c om_file.h om_file_funcproc_cb.c $(om_file_la_BUILTSOURCES)
om_file_la_LDFLAGS	= -module -no-undefined -avoid-version
om_file_la_LIBADD	= $(LIBNX)
om_filedir		= $(NX_MODULEDIR)/output

om_file_la_BUILTSOURCES = expr-om_file-funcproc.c expr-om_file-funcproc.h
expr-om_file-funcproc.c: $(top_builddir)/src/utils/codegen.pl om_file-api.xml
	-$(top_builddir)/src/utils/codegen.pl om_file-api.xml
expr-om_file-funcproc.h: $(top_builddir)/src/utils/codegen.pl om_file-api.xml
	-$(top_builddir)/src/utils/codegen.pl om_file-api.xml

EXTRA_DIST		= om_file-api.xml $(om_file_la_BUILTSOURCES)
MAINTAINERCLEANFILES	+= $(om_file_la_BUILTSOURCES)
