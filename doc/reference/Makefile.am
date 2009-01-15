MAINTAINERCLEANFILE = Makefile.in

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

# Images to copy into HTML directory
HTML_IMAGES =

# Extra SGML files that are included by $(DOC_MAIN_SGML_FILE)
content_files =			\
	version.xml		\
	readme.xml		\
	news.xml		\
	introduction.xml	\
	configuration.xml	\
	install.xml		\
	install-to-ubuntu.xml	\
	install-to-freebsd.xml

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
	install-to-freebsd.xml			\
	install-to-freebsd.xml.ja

CLEANFILES +=				\
	$(generated_xml_files)

# Other files to distribute
EXTRA_DIST +=				\
	html-build.stamp		\
	version.xml.in			\
	$(generated_xml_files)		\
	sf-logo.html			\
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

install-to-freebsd.xml: $(top_srcdir)/doc/install-to-freebsd.rd $(RD2REFENTRY_LIB) install-to-freebsd.xml.ja
	$(RD2) $(top_srcdir)/doc/install-to-freebsd.rd > install-to-freebsd.xml

install-to-freebsd.xml.ja: $(top_srcdir)/doc/install-to-freebsd.rd.ja $(RD2REFENTRY_LIB) ja.po
	$(RD2_JA) $(top_srcdir)/doc/install-to-freebsd.rd.ja > install-to-freebsd.xml.ja

upload: docs
	rm -rf tmp/
	mkdir -p tmp/
	cp -rp html/ tmp/reference
	for catalog in $(CATALOGS); do					\
	  lang=`echo $$catalog | sed 's/.po$$//'`;			\
	  cp -rp $$lang/html/ tmp/reference/$$lang/;			\
	done
	chmod -R g+w tmp/reference
	ruby -i'' -pe '$$_.sub!(/<\/body>/u,				\
	                       	File.read("sf-logo.html") + "</body>")'	\
	  tmp/reference/*.html						\
	  tmp/reference/*/*.html
	rsync -avz tmp/reference/ $(SF_HTDOCS)/reference/
	ssh $(SF_HOST) chmod -R g+w $(SF_HTDOCS_PATH)/reference/
