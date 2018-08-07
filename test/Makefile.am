SUBDIRS		= common coresrc
all-local:	modules-stamp
test_scripts	= conf-tester.sh module-tester.sh
TESTS		= $(test_scripts)

TEST_FILES	= $(shell find modules/ -type f |grep -v \.svn)
TEST_FILES	+= $(shell find conf/ -type f |grep -v \.svn)
EXTRA_DIST	= $(test_scripts) tester.pl conftest.txt test.log \
	          $(TEST_FILES) common.conf $(wildcard cert/*.pem)

modules-stamp:
	-rm -f $(top_srcdir)/src/modules/extension/*.so
	-rm -f $(top_srcdir)/src/modules/input/*.so
	-rm -f $(top_srcdir)/src/modules/processor/*.so
	-rm -f $(top_srcdir)/src/modules/output/*.so
	-cd $(top_srcdir)/src/modules/extension && find . -name "xm_*.so" -exec ln -s {} \;
	-cd $(top_srcdir)/src/modules/input && find . -name "im_*.so" -exec ln -s {} \;
	-cd $(top_srcdir)/src/modules/processor && find . -name "pm_*.so" -exec ln -s {} \;
	-cd $(top_srcdir)/src/modules/output && find . -name "om_*.so" -exec ln -s {} \;
