im_ssl_LTLIBRARIES	= im_ssl.la
im_ssl_la_SOURCES	= im_ssl.c im_ssl.h
im_ssl_la_LDFLAGS	= -module -no-undefined -avoid-version
im_ssl_la_LIBADD	= $(LIBNX) $(top_builddir)/src/common/libnxssl.la
im_ssldir		= $(NX_MODULEDIR)/input
dist_pkgdata_DATA	= im_ssl-fields.xml
