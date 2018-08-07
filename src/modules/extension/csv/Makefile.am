noinst_LTLIBRARIES		= libcsv.la
xm_csv_LTLIBRARIES		= xm_csv.la
xm_csv_la_SOURCES		= xm_csv.c xm_csv.h xm_csv_funcproc_cb.c $(xm_csv_la_BUILTSOURCES)
libcsv_la_SOURCES		= csv.c csv.h
xm_csv_la_LDFLAGS		= -module -no-undefined -avoid-version
xm_csv_la_LIBADD		= libcsv.la $(LIBNX)
xm_csvdir			= $(NX_MODULEDIR)/extension
xm_csv_la_BUILTSOURCES		= expr-xm_csv-funcproc.c expr-xm_csv-funcproc.h
expr-xm_csv-funcproc.c: $(top_builddir)/src/utils/codegen.pl xm_csv-api.xml
	-$(top_builddir)/src/utils/codegen.pl xm_csv-api.xml
expr-xm_csv-funcproc.h: $(top_builddir)/src/utils/codegen.pl xm_csv-api.xml
	-$(top_builddir)/src/utils/codegen.pl xm_csv-api.xml

EXTRA_DIST		= xm_csv-api.xml $(xm_csv_la_BUILTSOURCES)
MAINTAINERCLEANFILES	+= $(xm_csv_la_BUILTSOURCES)
