pm_transformer_LTLIBRARIES	= pm_transformer.la
pm_transformer_la_SOURCES	= pm_transformer.c pm_transformer.h
pm_transformer_la_LDFLAGS	= -module -no-undefined -avoid-version
pm_transformer_la_LIBADD	= $(top_srcdir)/src/modules/extension/syslog/libsyslog.la \
                                  $(top_srcdir)/src/modules/extension/csv/libcsv.la \
                                  $(top_srcdir)/src/modules/extension/json/libjson.la \
                                  $(top_srcdir)/src/modules/extension/xml/libxml.la \
	                          $(LIBNX)
pm_transformerdir		= $(NX_MODULEDIR)/processor
pm_transformer_la_DEPENDENCIES	= $(top_srcdir)/src/modules/extension/syslog/libsyslog.la \
                                  $(top_srcdir)/src/modules/extension/csv/libcsv.la \
                                  $(top_srcdir)/src/modules/extension/xml/libxml.la \
                                  $(top_srcdir)/src/modules/extension/json/libjson.la

$(top_srcdir)/src/modules/extension/syslog/libsyslog.la:
	cd $(top_srcdir)/src/modules/extension/syslog && $(MAKE)
$(top_srcdir)/src/modules/extension/csv/libcsv.la:
	cd $(top_srcdir)/src/modules/extension/csv && $(MAKE)
$(top_srcdir)/src/modules/extension/json/libjson.la:
	cd $(top_srcdir)/src/modules/extension/json && $(MAKE)
$(top_srcdir)/src/modules/extension/xml/libxml.la:
	cd $(top_srcdir)/src/modules/extension/xml && $(MAKE)
