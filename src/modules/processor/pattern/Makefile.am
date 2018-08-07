pm_pattern_LTLIBRARIES	= pm_pattern.la
pm_pattern_la_LDFLAGS	= -module -no-undefined -avoid-version
pm_pattern_la_LIBADD	= $(LIBNX) $(LIBEXPAT) $(PCRE_LIBS)
pm_pattern_CFLAGS	= $(PCRE_CFLAGS)
pm_patterndir		= $(NX_MODULEDIR)/processor
EXTRA_DIST		= patterndb_parser.xcc
dist_pkgdata_DATA	= pm_pattern-fields.xml
SUFFIXES		= .xcc
.xcc.c:
	-xcc -b -i $< -o $@
BUILT_SOURCES		= patterndb_parser.c
pm_pattern_la_SOURCES	= pm_pattern.c pm_pattern.h patterndb.c patterndb.h $(BUILT_SOURCES)
