bin_PROGRAMS		= nxlog-processor nxlog-stmnt-verifier
noinst_PROGRAMS		= stresstest
stresstest_SOURCES	= stresstest.c testinput.h
nxlog_processor_SOURCES	= processor.c
nxlog_stmnt_verifier_SOURCES = stmnt-verifier.c
stresstest_LDADD	= $(top_builddir)/src/common/libnx.la $(top_builddir)/src/core/libnxcore.la
nxlog_processor_LDADD	= $(top_builddir)/src/common/libnx.la \
                          $(top_builddir)/src/core/libnxlog.la \
                          $(top_builddir)/src/core/libnxcore.la
nxlog_stmnt_verifier_LDADD = $(top_builddir)/src/common/libnx.la \
                             $(top_builddir)/src/core/libnxcore.la \
		             $(top_builddir)/src/core/libnxlog.la
CFLAGS			+= -DNX_MODULEDIR=\"$(NX_MODULEDIR)\" -DNX_CACHEDIR=\"$(NX_CACHEDIR)\" -DNX_CONFIGFILE=\"$(NX_CONFIGFILE)\"

EXTRA_DIST		= codegen.pl
