if HAVE_PERL
xm_perl_LTLIBRARIES	= xm_perl.la
xm_perl_la_BUILTSOURCES	= expr-xm_perl-funcproc.c expr-xm_perl-funcproc.h 
xm_perl_la_SOURCES	= xm_perl.c xm_perl.h xm_perl_funcproc_cb.c libnxperl.xs $(xm_perl_la_BUILTSOURCES)
# in this case we don't include stuff in the src tarball because perl will be needed anyway for this and these generated files depend on the actual perl version
nodist_xm_perl_la_SOURCES = libnxperl.c ppport.h xsinit.c
xm_perl_la_LDFLAGS	= -module -no-undefined -avoid-version
xm_perl_la_LIBADD	= $(LIBNX) $(PERL_LIBS)
xm_perl_la_CFLAGS	= $(PERL_CFLAGS)
xm_perldir		= $(NX_MODULEDIR)/extension
TYPEMAP=`./findtypemap.pl`
MODPATH=`./findinstdir.pl`
expr-xm_perl-funcproc.c: $(top_builddir)/src/utils/codegen.pl xm_perl-api.xml
	-$(top_builddir)/src/utils/codegen.pl xm_perl-api.xml
expr-xm_perl-funcproc.h: $(top_builddir)/src/utils/codegen.pl xm_perl-api.xml
	-$(top_builddir)/src/utils/codegen.pl xm_perl-api.xml
libnxperl.c: typemap libnxperl.xs ppport.h
	xsubpp -noprototypes -typemap $(TYPEMAP) -typemap typemap libnxperl.xs > libnxperl.c
ppport.h:
	perl -MDevel::PPPort -eDevel::PPPort::WriteFile
xsinit.c: 
	perl -MExtUtils::Embed -e xsinit -- -o xsinit.c DynaLoader Log::Nxlog
dist_pm_DATA		= Log/Nxlog.pm
pmdir			= $(MODPATH)/Log

endif
EXTRA_DIST		= xm_perl-api.xml libnxperl.xs typemap findtypemap.pl findinstdir.pl $(xm_perl_la_BUILTSOURCES)
MAINTAINERCLEANFILES	+= $(xm_perl_la_BUILTSOURCES)
