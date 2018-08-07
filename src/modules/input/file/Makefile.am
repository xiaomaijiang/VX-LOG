im_file_LTLIBRARIES	= im_file.la
im_file_la_SOURCES	= im_file.c im_file.h im_file_funcproc_cb.c $(im_file_la_BUILTSOURCES)
im_file_la_LDFLAGS	= -module -no-undefined -avoid-version
im_file_la_LIBADD	= $(LIBNX)
im_filedir		= $(NX_MODULEDIR)/input

im_file_la_BUILTSOURCES = expr-im_file-funcproc.c expr-im_file-funcproc.h
expr-im_file-funcproc.c: $(top_builddir)/src/utils/codegen.pl im_file-api.xml
	-$(top_builddir)/src/utils/codegen.pl im_file-api.xml
expr-im_file-funcproc.h: $(top_builddir)/src/utils/codegen.pl im_file-api.xml
	-$(top_builddir)/src/utils/codegen.pl im_file-api.xml

EXTRA_DIST		= im_file-api.xml $(im_file_la_BUILTSOURCES)
MAINTAINERCLEANFILES	+= $(im_file_la_BUILTSOURCES)
