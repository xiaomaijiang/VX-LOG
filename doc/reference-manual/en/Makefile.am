if BUILD_DOCUMENTATION
all-local: nxlog-reference-manual.html nxlog-reference-manual.pdf nxlog.8 nxlog-processor.8
nxlog-reference-manual.html: $(DOC_SRC) ../config-examples/stamp-examples stamp-apidoc stamp-fields version.txt
	-asciidoctor -a productVersion=`cat version.txt` -a month=`date -u +%B` -a year=`date -u +%Y` -r ./block.rb nxlog-reference-manual.adoc
nxlog-reference-manual.pdf: $(DOC_SRC) ../config-examples/stamp-examples stamp-apidoc stamp-fields version.txt
	-asciidoctor-pdf -a productVersion=`cat version.txt` -a month=`date -u +%B` -a year=`date -u +%Y` -r ./block.rb nxlog-reference-manual.adoc
nxlog.8: $(DOC_SRC) version.txt
	-asciidoctor -a productVersion=`cat version.txt` -a month=`date -u +%B` -a year=`date -u +%Y` -b manpage -o nxlog.8 man_nxlog.adoc
nxlog-processor.8: $(DOC_SRC) version.txt
	-asciidoctor -a productVersion=`cat version.txt` -a month=`date -u +%B` -a year=`date -u +%Y` -b manpage -o nxlog-processor.8 man_nxlog-processor.adoc
man_MANS = nxlog.8 nxlog-processor.8
clean-docs:
	-rm -f nxlog-reference-manual.html
	-rm -f nxlog-reference-manual.pdf
	-rm -f nxlog.8
	-rm -f nxlog-processor.8
	-rm -f apidoc-*.adoc
	-rm -f fields-*.adoc
	-rm -f stamp-*
else
all-local: 
	echo "not building documentation (disabled by configure)"
clean-docs: 
	echo "not cleaning documentation (disabled by configure)"
endif


clean-local: clean-docs

version.txt: ../../../VERSION ../../../svn_version.txt
	echo `cat ../../../VERSION`.`cat ../../../svn_version.txt` >version.txt

../config-examples/stamp-examples: $(wildcard ../config-examples/*.conf) \
                                   $(wildcard ../config-examples/*.stmnt) \
                                   ../../../test/modules/processor/pattern/patterndb.xml
	cd ../config-examples && $(MAKE) stamp-examples


CHAPTERS = language.adoc configuration.adoc modules.adoc

MODULES = $(wildcard modules/extension/*.adoc) \
          $(wildcard modules/input/*.adoc) \
          $(wildcard modules/output/*.adoc) \
          $(wildcard modules/processor/*.adoc)

APIDOC_SRC := $(wildcard ../../../src/modules/*/*/*-api.xml) ../../../src/common/core-api.xml
stamp-apidoc: apidoc.sh $(APIDOC_SRC)
	-./apidoc.sh && touch stamp-apidoc

stamp-fields: fields.sh $(wildcard ../../../src/modules/*/*/*-fields.xml) ../../../src/common/core-fields.xml
	-./fields.sh && touch stamp-fields

DOC_SRC = nxlog-reference-manual.adoc $(CHAPTERS) $(MODULES) \
          $(wildcard ../config-examples/*.conf) \
          $(wildcard ../config-examples/*.stmnt) $(figures_DATA) \
          ../../../svn_version.txt ../../../VERSION


docbook_DATA	= nxlog-reference-manual.html nxlog-reference-manual.pdf
docbookdir	= $(docdir)
figures_DATA	= $(wildcard ../figures/*.png)
figuresdir	= $(docdir)/figures

EXTRA_DIST = $(wildcard *.adoc) $(docbook_DATA) $(CHAPTERS) $(MODULES) $(APIDOC) \
             apidoc.sh stamp-apidoc fields.sh stamp-fields
dist-hook:
	cp $(top_srcdir)/doc/reference-manual/en/*.adoc $(distdir)/


maintainer-clean-local:
	-$(RM) $(docbook_DATA) $(wildcard apidoc-*.adoc) stamp-apidoc \
	$(wildcard fields-*.adoc) stamp-fields version.txt
