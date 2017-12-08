## Process this file with automake to produce Makefile.in
SUBDIRS = . devhelp examples

# The top-level SGML file.
DOC_MAIN_XML_FILE=gnome-xml.xml

# The directory containing the source code (if it contains documentation).
DOC_SOURCE_DIR=..

# A file in win32 depends upon one of the doc files
WIN32_DIR=$(top_srcdir)/win32

PAGES= architecture.html bugs.html contribs.html docs.html DOM.html \
    downloads.html entities.html example.html help.html index.html \
    interface.html intro.html library.html namespaces.html news.html \
    tree.html xmldtd.html XMLinfo.html XSLT.html
APIPAGES=APIconstructors.html APIfiles.html APIfunctions.html \
    APIsymbols.html APIchunk0.html

if REBUILD_DOCS
EXTRA_DIST_wc = xmlcatalog_man.xml $(wildcard tutorial/*.html) \
    $(wildcard tutorial/*.c) $(wildcard tutorial/*.pdf) \
    $(wildcard tutorial/images/*.png) \
    $(wildcard tutorial/images/callouts/*.png) $(wildcard API*.html) \
    $(wildcard *.1) $(wildcard *.xsl) $(wildcard *.html) \
    $(wildcard *.gif) w3c.png $(wildcard html/*.html) \
    $(wildcard html/*.png) libxml2-api.xml index.py search.php \
    apibuild.py libxml2.xsa xmllint.xml xmlcatalog_man.xml \
    README.docs symbols.xml
endif

# Expanded form of EXTRA_DIST_wc
#
EXTRA_DIST = \
	APIchunk0.html \
	APIchunk1.html \
	APIchunk2.html \
	APIchunk3.html \
	APIchunk4.html \
	APIchunk5.html \
	APIchunk6.html \
	APIchunk7.html \
	APIchunk8.html \
	APIchunk9.html \
	APIchunk10.html \
	APIchunk11.html \
	APIchunk12.html \
	APIchunk13.html \
	APIchunk14.html \
	APIchunk15.html \
	APIchunk16.html \
	APIchunk17.html \
	APIchunk18.html \
	APIchunk19.html \
	APIchunk20.html \
	APIchunk21.html \
	APIchunk22.html \
	APIchunk23.html \
	APIchunk24.html \
	APIchunk25.html \
	APIchunk26.html \
	APIchunk27.html \
	APIchunk28.html \
	APIchunk29.html \
	APIconstructors.html \
	APIfiles.html \
	APIfunctions.html \
	APIsymbols.html \
	ChangeLog.xsl \
	DOM.gif \
	DOM.html \
	FAQ.html \
	Libxml2-Logo-180x168.gif \
	Libxml2-Logo-90x34.gif \
	README.docs \
	XMLinfo.html \
	XSLT.html \
	api.xsl \
	apibuild.py \
	architecture.html \
	bugs.html \
	catalog.gif \
	catalog.html \
	checkapisym.xsl \
	contribs.html \
	docs.html \
	downloads.html \
	elfgcchack.xsl \
	encoding.html \
	entities.html \
	example.html \
	guidelines.html \
	help.html \
	html/book1.html \
	html/home.png \
	html/index.html \
	html/left.png \
	html/libxml-DOCBparser.html \
	html/libxml-HTMLparser.html \
	html/libxml-HTMLtree.html \
	html/libxml-SAX.html \
	html/libxml-SAX2.html \
	html/libxml-c14n.html \
	html/libxml-catalog.html \
	html/libxml-chvalid.html \
	html/libxml-debugXML.html \
	html/libxml-dict.html \
	html/libxml-encoding.html \
	html/libxml-entities.html \
	html/libxml-globals.html \
	html/libxml-hash.html \
	html/libxml-lib.html \
	html/libxml-list.html \
	html/libxml-nanoftp.html \
	html/libxml-nanohttp.html \
	html/libxml-parser.html \
	html/libxml-parserInternals.html \
	html/libxml-pattern.html \
	html/libxml-relaxng.html \
	html/libxml-schemasInternals.html \
	html/libxml-schematron.html \
	html/libxml-threads.html \
	html/libxml-tree.html \
	html/libxml-uri.html \
	html/libxml-valid.html \
	html/libxml-xinclude.html \
	html/libxml-xlink.html \
	html/libxml-xmlIO.html \
	html/libxml-xmlautomata.html \
	html/libxml-xmlerror.html \
	html/libxml-xmlexports.html \
	html/libxml-xmlmemory.html \
	html/libxml-xmlmodule.html \
	html/libxml-xmlreader.html \
	html/libxml-xmlregexp.html \
	html/libxml-xmlsave.html \
	html/libxml-xmlschemas.html \
	html/libxml-xmlschemastypes.html \
	html/libxml-xmlstring.html \
	html/libxml-xmlunicode.html \
	html/libxml-xmlversion.html \
	html/libxml-xmlwriter.html \
	html/libxml-xpath.html \
	html/libxml-xpathInternals.html \
	html/libxml-xpointer.html \
	html/libxml-xzlib.html \
	html/right.png \
	html/up.png \
	index.html \
	index.py \
	interface.html \
	intro.html \
	library.html \
	libxml.gif \
	libxml2-api.xml \
	libxml2.xsa \
	namespaces.html \
	newapi.xsl \
	news.html \
	news.xsl \
	python.html \
	redhat.gif \
	search.php \
	searches.html \
	searches.xsl \
	site.xsl \
	smallfootonly.gif \
	structure.gif \
	symbols.xml \
	syms.xsl \
	threads.html \
	tree.html \
	tutorial/apa.html \
	tutorial/apb.html \
	tutorial/apc.html \
	tutorial/apd.html \
	tutorial/ape.html \
	tutorial/apf.html \
	tutorial/apg.html \
	tutorial/aph.html \
	tutorial/api.html \
	tutorial/ar01s02.html \
	tutorial/ar01s03.html \
	tutorial/ar01s04.html \
	tutorial/ar01s05.html \
	tutorial/ar01s06.html \
	tutorial/ar01s07.html \
	tutorial/ar01s08.html \
	tutorial/ar01s09.html \
	tutorial/images/blank.png \
	tutorial/images/callouts/1.png \
	tutorial/images/callouts/10.png \
	tutorial/images/callouts/2.png \
	tutorial/images/callouts/3.png \
	tutorial/images/callouts/4.png \
	tutorial/images/callouts/5.png \
	tutorial/images/callouts/6.png \
	tutorial/images/callouts/7.png \
	tutorial/images/callouts/8.png \
	tutorial/images/callouts/9.png \
	tutorial/images/caution.png \
	tutorial/images/draft.png \
	tutorial/images/home.png \
	tutorial/images/important.png \
	tutorial/images/next.png \
	tutorial/images/note.png \
	tutorial/images/prev.png \
	tutorial/images/tip.png \
	tutorial/images/toc-blank.png \
	tutorial/images/toc-minus.png \
	tutorial/images/toc-plus.png \
	tutorial/images/up.png \
	tutorial/images/warning.png \
	tutorial/includeaddattribute.c \
	tutorial/includeaddkeyword.c \
	tutorial/includeconvert.c \
	tutorial/includegetattribute.c \
	tutorial/includekeyword.c \
	tutorial/includexpath.c \
	tutorial/index.html \
	tutorial/ix01.html \
	tutorial/xmltutorial.pdf \
	upgrade.html \
	w3c.png \
	wiki.xsl \
	xml.html \
	xmlcatalog.1 \
	xmlcatalog_man.html \
	xmlcatalog_man.xml \
	xmldtd.html \
	xmlio.html \
	xmllint.1 \
	xmllint.html \
	xmllint.xml \
	xmlmem.html \
	xmlreader.html \
	xsa.xsl

man_MANS = xmllint.1 xmlcatalog.1

if REBUILD_DOCS
docs: web $(top_builddir)/NEWS libxml2.xsa $(man_MANS)

api: libxml2-api.xml libxml2-refs.xml $(APIPAGES) $(srcdir)/html/index.html $(WIN32_DIR)/libxml2.def.src ../elfgcchack.h $(srcdir)/site.xsl

web: $(PAGES)

../elfgcchack.h: $(srcdir)/elfgcchack.xsl $(srcdir)/libxml2-api.xml
	-@(if [ -x $(XSLTPROC) ] ; then \
	  echo "Rebuilding the elfgcchack.h header" ; \
	  $(XSLTPROC) --nonet $(srcdir)/elfgcchack.xsl $(srcdir)/libxml2-api.xml > elfgcchack.h ; \
	  if [ "`diff -q elfgcchack.h ../elfgcchack.h`" ] ; then \
	    echo "updating ../elfgcchack.h"; \
	    cp elfgcchack.h ../elfgcchack.h; \
	  fi ; rm -f elfgcchack.h ;  fi );

$(PAGES): xml.html $(srcdir)/site.xsl
	-@(if [ -x $(XSLTPROC) ] ; then \
	  echo "Rebuilding the HTML Web pages from xml.html" ; \
	  $(XSLTPROC) --nonet --html --path $(srcdir) $(srcdir)/site.xsl $(srcdir)/xml.html > index.html ; fi );
	-@(if [ -x $(XMLLINT) ] ; then \
	  echo "Validating the HTML Web pages" ; \
	  $(XMLLINT) --nonet --valid --noout $(PAGES) ; fi );

$(top_builddir)/NEWS: $(srcdir)/news.xsl $(srcdir)/news.html
	-@(if [ -x $(XSLTPROC) ] ; then \
	  $(XSLTPROC) --nonet $(srcdir)/news.xsl $(srcdir)/news.html > $(top_builddir)/NEWS ; fi );

libxml2.xsa: $(srcdir)/xsa.xsl $(srcdir)/news.html
	-@(if [ -x $(XSLTPROC) ] ; then \
	  echo "Rebuilding the NEWS file" ; \
	  $(XSLTPROC) --nonet $(srcdir)/xsa.xsl $(srcdir)/news.html > libxml2.xsa ; fi );

$(APIPAGES): libxml2-api.xml libxml2-refs.xml $(srcdir)/site.xsl $(srcdir)/api.xsl
	-@(if [ -x $(XSLTPROC) ] ; then \
	  echo "Rebuilding the HTML API pages from libxml2-refs.xml" ; \
	  $(XSLTPROC) --nonet --html $(srcdir)/api.xsl \
	                     $(srcdir)/xml.html ; fi );
	-@(if [ -x $(XMLLINT) ] ; then \
	  echo "Validating the HTML API pages" ; \
	  $(XMLLINT) --nonet --valid --noout API*.html ; fi );

$(srcdir)/html/index.html: libxml2-api.xml $(srcdir)/newapi.xsl
	-@(if [ -x $(XSLTPROC) ] ; then \
	  echo "Rebuilding the HTML pages from the XML API" ; \
	  $(XSLTPROC) --nonet $(srcdir)/newapi.xsl $(srcdir)/libxml2-api.xml ; fi )
	-@(if [ -x $(XMLLINT) ] ; then \
	  echo "Validating the resulting XHTML pages" ; \
	  $(XMLLINT) --nonet --valid --noout html/*.html ; fi );

wiki: libxml2-api.xml $(srcdir)/wiki.xsl
	-@(if [ -x $(XSLTPROC) ] ; then \
		echo "Rebuilding the wiki HTML pages from the XML API" ; \
		$(XSLTPROC) --nonet $(srcdir)/wiki.xsl $(srcdir)/libxml2-api.xml; fi )

$(WIN32_DIR)/libxml2.def.src: libxml2-api.xml
	-@(if [ -x $(XSLTPROC) ] ; then \
	  $(XSLTPROC) -o $(WIN32_DIR)/libxml2.def.src \
	  --nonet $(WIN32_DIR)/defgen.xsl libxml2-api.xml ; fi )

source_file_deps = \
	$(filter-out %/xmlversion.h, $(wildcard $(top_srcdir)/include/libxml/*.h)) \
	$(top_srcdir)/include/libxml/xmlversion.h.in \
	$(wildcard $(top_srcdir)/*.c)

libxml2-api.xml libxml2-refs.xml ../libxml2.syms: apibuild.py symbols.xml syms.xsl checkapisym.xsl $(source_file_deps)
	test -f $(top_srcdir)/include/libxml/xmlversion.h
	(cd $(srcdir) && ./apibuild.py)
	($(XSLTPROC) $(srcdir)/checkapisym.xsl $(srcdir)/libxml2-api.xml)
	($(XSLTPROC) -o ../libxml2.syms $(srcdir)/syms.xsl $(srcdir)/symbols.xml)
	-@(cd .. ; $(MAKE) rebuild_testapi)


xmllint.1: xmllint.xml
	-@($(XSLTPROC) --nonet xmllint.xml)

xmlcatalog.1: xmlcatalog_man.xml
	-@($(XSLTPROC) --nonet xmlcatalog_man.xml)

check-extra-dist:
	for f in $(EXTRA_DIST_wc) ; do echo $$f; done | sort -u >tmp.EXTRA_DIST_wc
	for f in $(EXTRA_DIST) ;    do echo $$f; done | sort    >tmp.EXTRA_DIST
	diff -u tmp.EXTRA_DIST_wc tmp.EXTRA_DIST
	rm -f tmp.EXTRA_DIST_wc tmp.EXTRA_DIST
endif

clean-local:
	rm -f *~ *.bak *.hierarchy *.signals *-unused.txt

maintainer-clean-local: clean-local
	rm -rf libxml-decl-list.txt libxml-decl.txt

rebuild: api docs

install-data-local:
	$(MKDIR_P) $(DESTDIR)$(HTML_DIR)
	-$(INSTALL) -m 0644 $(srcdir)/xml.html $(srcdir)/encoding.html $(srcdir)/FAQ.html $(srcdir)/structure.gif $(srcdir)/DOM.gif $(srcdir)/smallfootonly.gif $(srcdir)/redhat.gif $(srcdir)/libxml.gif $(srcdir)/w3c.png $(srcdir)/Libxml2-Logo-180x168.gif $(srcdir)/Libxml2-Logo-90x34.gif $(DESTDIR)$(HTML_DIR)
	$(MKDIR_P) $(DESTDIR)$(HTML_DIR)/html
	-$(INSTALL) -m 0644 $(srcdir)/html/*.html $(DESTDIR)$(HTML_DIR)/html
	-$(INSTALL) -m 0644 $(srcdir)/html/*.png $(DESTDIR)$(HTML_DIR)/html
	$(MKDIR_P) $(DESTDIR)$(HTML_DIR)/tutorial
	-$(INSTALL) -m 0644 $(srcdir)/tutorial/*.* \
		$(DESTDIR)$(HTML_DIR)/tutorial
	$(MKDIR_P) $(DESTDIR)$(HTML_DIR)/tutorial/images
	-$(INSTALL) -m 0644 $(srcdir)/tutorial/images/*.* \
		$(DESTDIR)$(HTML_DIR)/tutorial/images
	$(MKDIR_P) $(DESTDIR)$(HTML_DIR)/tutorial/images/callouts
	-$(INSTALL) -m 0644 $(srcdir)/tutorial/images/callouts/*.* \
		$(DESTDIR)$(HTML_DIR)/tutorial/images/callouts

.PHONY: docs api web wiki rebuild
