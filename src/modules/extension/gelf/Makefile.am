if HAVE_ZLIB
xm_gelf_LTLIBRARIES	= xm_gelf.la
xm_gelf_la_SOURCES	= gelf.c gelf.h xm_gelf.c xm_gelf.h
xm_gelf_la_LDFLAGS	= -module -no-undefined -avoid-version
xm_gelfdir		= $(NX_MODULEDIR)/extension
xm_gelf_la_LIBADD	= $(LIBZLIB) $(top_srcdir)/src/modules/extension/json/yajl/libyajl.la $(LIBNX)

$(top_srcdir)/src/modules/extension/json/yajl/libyajl.la:
	cd $(top_srcdir)/src/modules/extension/json/yajl && $(MAKE)
endif