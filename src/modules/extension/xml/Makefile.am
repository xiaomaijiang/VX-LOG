noinst_LTLIBRARIES	= libxml.la
libxml_la_SOURCES	= xml.c xml.h
libxml_la_LIBADD	= $(LIBEXPAT)
xm_xml_LTLIBRARIES	= xm_xml.la
xm_xml_la_SOURCES	= xm_xml.c xm_xml.h xm_xml_funcproc_cb.c $(xm_xml_la_BUILTSOURCES)
xm_xml_la_LDFLAGS	= -module -no-undefined -avoid-version
xm_xmldir		= $(NX_MODULEDIR)/extension
xm_xml_la_LIBADD	= libxml.la $(LIBNX)
xm_xml_la_BUILTSOURCES	= expr-xm_xml-funcproc.c expr-xm_xml-funcproc.h
expr-xm_xml-funcproc.c: $(top_builddir)/src/utils/codegen.pl xm_xml-api.xml
	-$(top_builddir)/src/utils/codegen.pl xm_xml-api.xml
expr-xm_xml-funcproc.h: $(top_builddir)/src/utils/codegen.pl xm_xml-api.xml
	-$(top_builddir)/src/utils/codegen.pl xm_xml-api.xml

EXTRA_DIST		= xm_xml-api.xml $(xm_xml_la_BUILTSOURCES)
MAINTAINERCLEANFILES	+= $(xm_xml_la_BUILTSOURCES)
