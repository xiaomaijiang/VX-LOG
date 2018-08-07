test_programs	= date logdata value-serialize logdata-serialize expression-test \
                  str-test scheduler-test configcache value-test alloc-test
test_scripts	= stmnt-test.sh
TESTS		= $(test_programs) $(test_scripts)
noinst_PROGRAMS	= $(test_programs) stmnt-test
#csv_LDADD	= $(top_builddir)/src/modules/processor/transformer/libcsv.la \
#                  $(top_builddir)/src/common/libnx.la \
#                  $(top_builddir)/src/core/libnxcore.la

LDADD		= $(top_builddir)/src/common/libnx.la \
                  $(top_builddir)/src/core/libnxcore.la \
                  $(top_builddir)/src/core/libnxlog.la
#CFLAGS		+= -DNX_MODULEDIR=\"$(NX_MODULEDIR)\"

EXTRA_DIST	= $(test_scripts) $(wildcard statements/good/*) $(wildcard statements/bad/*) 
