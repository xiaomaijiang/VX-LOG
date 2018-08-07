SUBDIRS	= src test doc

maintainer-clean-local:
	-$(RM) aclocal.m4 configure config.h stamp-h1
	-$(RM) config.guess config.sub depcomp install-sh missing mkinstalldirs compile ltmain.sh ylwrap

changelog_DATA = ChangeLog.txt
changelogdir = $(docdir)

EXTRA_DIST = autogen.sh libtool svn_version.txt version.sh VERSION $(changelog_DATA) LICENSE LICENSE.pdf $(wildcard parsers/*.conf) \
             packaging/debian/changelog.skel \
	     packaging/debian/compat \
	     $(wildcard packaging/debian/control.*) \
	     packaging/debian/copyright \
	     packaging/debian/nxlog.init \
	     packaging/debian/make_debs.sh \
	     packaging/debian/nxlog.conf \
	     packaging/debian/nxlog-ce.install \
	     packaging/debian/watch \
	     packaging/debian/preinst \
	     packaging/debian/postinst \
	     packaging/debian/prerm \
	     packaging/debian/postrm \
	     packaging/debian/rules \
	     packaging/debian/source/format \
	     $(wildcard packaging/redhat/nxlog.spec.*) \
	     packaging/redhat/nxlog.conf \
	     packaging/redhat/init \
	     packaging/redhat/make_rpm.sh
