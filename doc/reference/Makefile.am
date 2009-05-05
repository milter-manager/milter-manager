MAINTAINERCLEANFILES = Makefile.in

# The name of the module.
DOC_MODULE = milter-manager

# The top-level SGML file.
DOC_MAIN_SGML_FILE = milter-manager-docs.sgml

# Extra options to supply to gtkdoc-scan
SCAN_OPTIONS =							\
	--deprecated-guards="MILTER_DISABLE_DEPRECATED"

# The directory containing the source code. Relative to $(srcdir)
DOC_SOURCE_DIR =	\
	../../milter	\
	../../libmilter

# Used for dependencies
HFILE_GLOB =					\
	$(top_srcdir)/milter/*.h		\
	$(top_srcdir)/milter/*/*.h		\
	$(top_srcdir)/libmilter/*.h
CFILE_GLOB=					\
	$(top_srcdir)/milter/*/*.c		\
	$(top_srcdir)/libmilter/*.c

# Headers to ignore
IGNORE_HFILES=					\
	milter-marshalers.h

INCLUDES =					\
	-I$(top_srcdir)/libmilter		\
	-I$(top_srcdir)				\
	-I$(top_builddir)			\
	$(GLIB_CFLAGS)

GTKDOC_LIBS =							\
	$(top_builddir)/milter/core/libmilter-core.la		\
	$(top_builddir)/milter/client/libmilter-client.la	\
	$(top_builddir)/milter/server/libmilter-server.la	\
	$(top_builddir)/milter/manager/libmilter-manager.la	\
	$(top_builddir)/libmilter/libmilter.la			\
	$(GLIB_LIBS)

# Extra options to supply to gtkdoc-mkdb
MKDB_OPTIONS=--sgml-mode --output-format=xml

generated_image_files =					\
	mta-and-milter-manager-and-child-milter.png	\
	mta-and-milter-manager-and-child-milter-ja.png	\
	sendmail-and-milter.png				\
	sendmail-and-milter-ja.png			\
	sendmail-and-postfix-and-milter.png		\
	sendmail-and-postfix-and-milter-ja.png		\
	milter-manager-log-analyzer-graphs.png		\
	milter-manager-admin.png			\
	milter-manager-admin-ja.png

# Images to copy into HTML directory
HTML_IMAGES = $(generated_image_files)

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE)
content_files =					\
	version.xml				\
	readme.xml				\
	news.xml				\
	introduction.xml			\
	configuration.xml			\
	install.xml				\
	install-to-ubuntu.xml			\
	install-to-centos.xml			\
	install-to-freebsd.xml			\
	install-to-others.xml			\
	install-options-to-ubuntu.xml		\
	install-options-to-centos.xml		\
	install-options-to-freebsd.xml		\
	milter-manager.xml			\
	milter-test-server.xml			\
	milter-test-client.xml			\
	milter-performance-check.xml		\
	milter-manager-log-analyzer.xml

# Extra options to supply to gtkdoc-fixxref
FIXXREF_OPTIONS=

CATALOGS=ja.po

# include common portion ...
include $(top_srcdir)/gtk-doc.make

generated_xml_files =				\
	readme.xml				\
	readme.xml.ja				\
	news.xml				\
	news.xml.ja				\
	introduction.xml			\
	introduction.xml.ja			\
	configuration.xml			\
	configuration.xml.ja			\
	install.xml				\
	install.xml.ja				\
	install-to-ubuntu.xml			\
	install-to-ubuntu.xml.ja		\
	install-to-centos.xml			\
	install-to-centos.xml.ja		\
	install-to-freebsd.xml			\
	install-to-freebsd.xml.ja		\
	install-to-others.xml			\
	install-to-others.xml.ja		\
	install-options-to-ubuntu.xml		\
	install-options-to-ubuntu.xml.ja	\
	install-options-to-centos.xml		\
	install-options-to-centos.xml.ja	\
	install-options-to-freebsd.xml		\
	install-options-to-freebsd.xml.ja	\
	milter-manager.xml			\
	milter-manager.xml.ja			\
	milter-test-server.xml			\
	milter-test-server.xml.ja		\
	milter-test-client.xml			\
	milter-test-client.xml.ja		\
	milter-performance-check.xml		\
	milter-performance-check.xml.ja		\
	milter-manager-log-analyzer.xml		\
	milter-manager-log-analyzer.xml.ja

MAINTAINERCLEANFILES +=			\
	$(generated_xml_files)		\
	$(generated_image_files)

# Other files to distribute
EXTRA_DIST +=				\
	html-build.stamp		\
	version.xml.in			\
	$(generated_xml_files)		\
	head.html			\
	head-ja.html			\
	header.html			\
	header-ja.html			\
	footer.html			\
	footer-ja.html			\
	ja.po

$(DOC_MODULE).pot: sgml-build.stamp
	xml2po -k -o $(DOC_MODULE).pot				\
	  $(srcdir)/$(DOC_MAIN_SGML_FILE) $(srcdir)/xml/*.xml

update-po: Makefile sgml.stamp
	$(MAKE) $(DOC_MODULE).pot
	for catalog in $(CATALOGS); do		\
	  xml2po -k -u $$catalog		\
	    $(srcdir)/$(DOC_MAIN_SGML_FILE)	\
	    $(srcdir)/xml/*.xml;		\
	done

EXPAND_RD2_SNIPPET = $(top_srcdir)/misc/expand-rd2-snippet.rb
RD2REFENTRY_LIB = $(top_srcdir)/misc/rd2refentry-lib.rb
RD2 = rd2 -r $(RD2REFENTRY_LIB) --name=MILTER-MANAGER
RD2_JA = $(RD2) --po=$(srcdir)/ja.po
SVG2PNG = inkscape --export-dpi=60 --export-background=white --export-png

### RD -> XML
readme.xml: $(top_srcdir)/README $(RD2REFENTRY_LIB) readme.xml.ja
	$(RD2) $(top_srcdir)/README > readme.xml

readme.xml.ja: $(top_srcdir)/README.ja $(RD2REFENTRY_LIB) ja.po
	$(RD2_JA) $(top_srcdir)/README.ja > readme.xml.ja

news.xml: $(top_srcdir)/NEWS $(RD2REFENTRY_LIB) news.xml.ja
	$(RD2) $(top_srcdir)/NEWS > news.xml

news.xml.ja: $(top_srcdir)/NEWS.ja $(RD2REFENTRY_LIB) ja.po
	$(RD2_JA) $(top_srcdir)/NEWS.ja > news.xml.ja

introduction.xml: $(top_srcdir)/doc/introduction.rd $(RD2REFENTRY_LIB) introduction.xml.ja
	$(RD2) $(top_srcdir)/doc/introduction.rd > introduction.xml

introduction.xml.ja: $(top_srcdir)/doc/introduction.rd.ja $(RD2REFENTRY_LIB) ja.po
	$(RD2_JA) $(top_srcdir)/doc/introduction.rd.ja > introduction.xml.ja

configuration.xml: $(top_srcdir)/doc/configuration.rd $(RD2REFENTRY_LIB) configuration.xml.ja
	$(RD2) $(top_srcdir)/doc/configuration.rd > configuration.xml

configuration.xml.ja: $(top_srcdir)/doc/configuration.rd.ja $(RD2REFENTRY_LIB) ja.po
	$(RD2_JA) $(top_srcdir)/doc/configuration.rd.ja > configuration.xml.ja

install.xml: $(top_srcdir)/doc/install.rd $(RD2REFENTRY_LIB) install.xml.ja
	$(RD2) $(top_srcdir)/doc/install.rd > install.xml

install.xml.ja: $(top_srcdir)/doc/install.rd.ja $(RD2REFENTRY_LIB) ja.po
	$(RD2_JA) $(top_srcdir)/doc/install.rd.ja > install.xml.ja

install-to-ubuntu.xml: $(top_srcdir)/doc/install-to-ubuntu.rd $(RD2REFENTRY_LIB) install-to-ubuntu.xml.ja
	$(RD2) $(top_srcdir)/doc/install-to-ubuntu.rd > install-to-ubuntu.xml

install-to-ubuntu.xml.ja: $(top_srcdir)/doc/install-to-ubuntu.rd.ja $(RD2REFENTRY_LIB) ja.po
	$(RD2_JA) $(top_srcdir)/doc/install-to-ubuntu.rd.ja > install-to-ubuntu.xml.ja

install-to-centos.xml: $(top_srcdir)/doc/install-to-centos.rd $(RD2REFENTRY_LIB) install-to-centos.xml.ja
	$(RD2) $(top_srcdir)/doc/install-to-centos.rd > install-to-centos.xml

install-to-centos.xml.ja: $(top_srcdir)/doc/install-to-centos.rd.ja $(RD2REFENTRY_LIB) ja.po
	$(RD2_JA) $(top_srcdir)/doc/install-to-centos.rd.ja > install-to-centos.xml.ja

install-to-freebsd.xml: $(top_srcdir)/doc/install-to-freebsd.rd $(RD2REFENTRY_LIB) install-to-freebsd.xml.ja
	$(RD2) $(top_srcdir)/doc/install-to-freebsd.rd > install-to-freebsd.xml

install-to-freebsd.xml.ja: $(top_srcdir)/doc/install-to-freebsd.rd.ja $(RD2REFENTRY_LIB) ja.po
	$(RD2_JA) $(top_srcdir)/doc/install-to-freebsd.rd.ja > install-to-freebsd.xml.ja

install-to-others.xml: $(top_srcdir)/doc/install-to-others.rd $(RD2REFENTRY_LIB) install-to-others.xml.ja
	$(RD2) $(top_srcdir)/doc/install-to-others.rd > install-to-others.xml

install-to-others.xml.ja: $(top_srcdir)/doc/install-to-others.rd.ja $(RD2REFENTRY_LIB) ja.po
	$(RD2_JA) $(top_srcdir)/doc/install-to-others.rd.ja > install-to-others.xml.ja

install-options-to-ubuntu.xml: $(top_srcdir)/doc/install-options-to-ubuntu.rd $(RD2REFENTRY_LIB) install-options-to-ubuntu.xml.ja
	$(RD2) $(top_srcdir)/doc/install-options-to-ubuntu.rd > install-options-to-ubuntu.xml

install-options-to-ubuntu.xml.ja: $(top_srcdir)/doc/install-options-to-ubuntu.rd.ja $(RD2REFENTRY_LIB) ja.po
	$(RD2_JA) $(top_srcdir)/doc/install-options-to-ubuntu.rd.ja > install-options-to-ubuntu.xml.ja

install-options-to-centos.xml: $(top_srcdir)/doc/install-options-to-centos.rd $(RD2REFENTRY_LIB) install-options-to-centos.xml.ja
	$(RD2) $(top_srcdir)/doc/install-options-to-centos.rd > install-options-to-centos.xml

install-options-to-centos.xml.ja: $(top_srcdir)/doc/install-options-to-centos.rd.ja $(RD2REFENTRY_LIB) ja.po
	$(RD2_JA) $(top_srcdir)/doc/install-options-to-centos.rd.ja > install-options-to-centos.xml.ja

install-options-to-freebsd.xml: $(top_srcdir)/doc/install-options-to-freebsd.rd $(RD2REFENTRY_LIB) install-options-to-freebsd.xml.ja
	$(RD2) $(top_srcdir)/doc/install-options-to-freebsd.rd > install-options-to-freebsd.xml

install-options-to-freebsd.xml.ja: $(top_srcdir)/doc/install-options-to-freebsd.rd.ja $(RD2REFENTRY_LIB) ja.po
	$(RD2_JA) $(top_srcdir)/doc/install-options-to-freebsd.rd.ja > install-options-to-freebsd.xml.ja

milter-manager.xml: $(top_srcdir)/doc/milter-manager.rd $(RD2REFENTRY_LIB) milter-manager.xml.ja
	$(RD2) $(top_srcdir)/doc/milter-manager.rd > milter-manager.xml

milter-manager.xml.ja: $(top_srcdir)/doc/milter-manager.rd.ja $(RD2REFENTRY_LIB) ja.po
	$(RD2_JA) $(top_srcdir)/doc/milter-manager.rd.ja > milter-manager.xml.ja

milter-test-server.xml: $(top_srcdir)/doc/milter-test-server.rd $(RD2REFENTRY_LIB) milter-test-server.xml.ja
	$(RD2) $(top_srcdir)/doc/milter-test-server.rd > milter-test-server.xml

milter-test-server.xml.ja: $(top_srcdir)/doc/milter-test-server.rd.ja $(RD2REFENTRY_LIB) ja.po
	$(RD2_JA) $(top_srcdir)/doc/milter-test-server.rd.ja > milter-test-server.xml.ja

milter-test-client.xml: $(top_srcdir)/doc/milter-test-client.rd $(RD2REFENTRY_LIB) milter-test-client.xml.ja
	$(RD2) $(top_srcdir)/doc/milter-test-client.rd > milter-test-client.xml

milter-test-client.xml.ja: $(top_srcdir)/doc/milter-test-client.rd.ja $(RD2REFENTRY_LIB) ja.po
	$(RD2_JA) $(top_srcdir)/doc/milter-test-client.rd.ja > milter-test-client.xml.ja

milter-performance-check.xml: $(top_srcdir)/doc/milter-performance-check.rd $(RD2REFENTRY_LIB) milter-performance-check.xml.ja
	$(RD2) $(top_srcdir)/doc/milter-performance-check.rd > milter-performance-check.xml

milter-performance-check.xml.ja: $(top_srcdir)/doc/milter-performance-check.rd.ja $(RD2REFENTRY_LIB) ja.po
	$(RD2_JA) $(top_srcdir)/doc/milter-performance-check.rd.ja > milter-performance-check.xml.ja

milter-manager-log-analyzer.xml: $(top_srcdir)/doc/milter-manager-log-analyzer.rd $(RD2REFENTRY_LIB) milter-manager-log-analyzer.xml.ja
	$(RD2) $(top_srcdir)/doc/milter-manager-log-analyzer.rd > milter-manager-log-analyzer.xml

milter-manager-log-analyzer.xml.ja: $(top_srcdir)/doc/milter-manager-log-analyzer.rd.ja $(RD2REFENTRY_LIB) ja.po
	$(RD2_JA) $(top_srcdir)/doc/milter-manager-log-analyzer.rd.ja > milter-manager-log-analyzer.xml.ja

### SVG -> PNG
mta-and-milter-manager-and-child-milter.png: $(top_srcdir)/doc/mta-and-milter-manager-and-child-milter.svg
	$(SVG2PNG) $@ $<

mta-and-milter-manager-and-child-milter-ja.png: $(top_srcdir)/doc/mta-and-milter-manager-and-child-milter-ja.svg
	$(SVG2PNG) $@ $<

sendmail-and-milter.png: $(top_srcdir)/doc/sendmail-and-milter.svg
	$(SVG2PNG) $@ $<

sendmail-and-milter-ja.png: $(top_srcdir)/doc/sendmail-and-milter-ja.svg
	$(SVG2PNG) $@ $<

sendmail-and-postfix-and-milter.png: $(top_srcdir)/doc/sendmail-and-postfix-and-milter.svg
	$(SVG2PNG) $@ $<

sendmail-and-postfix-and-milter-ja.png: $(top_srcdir)/doc/sendmail-and-postfix-and-milter-ja.svg
	$(SVG2PNG) $@ $<

milter-manager-log-analyzer-graphs.png: $(top_srcdir)/doc/milter-manager-log-analyzer-graphs.svg
	$(SVG2PNG) $@ $<

milter-manager-admin.png: $(top_srcdir)/doc/milter-manager-admin.svg
	$(SVG2PNG) $@ $<

milter-manager-admin-ja.png: $(top_srcdir)/doc/milter-manager-admin-ja.svg
	$(SVG2PNG) $@ $<


upload: docs
	rm -rf tmp/
	mkdir -p tmp/
	cp -rp html/ tmp/reference
	for catalog in $(CATALOGS); do					\
	  lang=`echo $$catalog | sed 's/.po$$//'`;			\
	  cp -rp $$lang/html/ tmp/reference/$$lang/;			\
	done
	chmod -R g+w tmp/reference
	ruby $(top_srcdir)/misc/prepare-gtk-doc-html-for-publishing.rb	\
	  --template-directory=$(srcdir) --title "$(PACKAGE_TITLE)"	\
	  tmp/reference/*.html
	ruby $(top_srcdir)/misc/prepare-gtk-doc-html-for-publishing.rb	\
	  --template-directory=$(srcdir) --title "$(PACKAGE_TITLE)"	\
	  --language=ja tmp/reference/ja/*.html
	rsync -avz --delete tmp/reference/ $(SF_HTDOCS)/reference/
