# -*- mode: makefile -*-

####################################
# Everything below here is generic #
####################################

if GTK_DOC_USE_LIBTOOL
GTKDOC_CC = $(LIBTOOL) --mode=compile $(CC) $(AM_CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
GTKDOC_LD = $(LIBTOOL) --mode=link $(CC) $(AM_CFLAGS) $(CFLAGS) $(LDFLAGS)
else
GTKDOC_CC = $(CC) $(AM_CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
GTKDOC_LD = $(CC) $(AM_CFLAGS) $(CFLAGS) $(LDFLAGS)
endif

# We set GPATH here; this gives us semantics for GNU make
# which are more like other make's VPATH, when it comes to
# whether a source that is a target of one rule is then
# searched for in VPATH/GPATH.
#
GPATH = $(srcdir)

TARGET_DIR=$(HTML_DIR)/$(DOC_MODULE)

MAIN_SGML_FILE = main.sgml

EXTRA_DIST = 				\
	$(content_files)		\
	$(HTML_IMAGES)			\
	$(MAIN_SGML_FILE)		\
	$(DOC_MAIN_SGML_FILE)		\
	$(DOC_MODULE)-sections.txt	\
	$(DOC_MODULE)-overrides.txt

DOC_STAMPS=scan-build.stamp tmpl-build.stamp sgml-build.stamp html-build.stamp \
	   $(srcdir)/tmpl.stamp $(srcdir)/sgml.stamp $(srcdir)/html.stamp

SCANOBJ_FILES = 		 \
	$(DOC_MODULE).args 	 \
	$(DOC_MODULE).hierarchy  \
	$(DOC_MODULE).interfaces \
	$(DOC_MODULE).prerequisites \
	$(DOC_MODULE).signals

REPORT_FILES = \
	$(DOC_MODULE)-undocumented.txt \
	$(DOC_MODULE)-undeclared.txt \
	$(DOC_MODULE)-unused.txt

CLEANFILES = $(SCANOBJ_FILES) $(REPORT_FILES) $(DOC_STAMPS)

if ENABLE_GTK_DOC
all-local: html-build.stamp
else
all-local:
endif

docs: html-build.stamp

#### scan ####

scan-build.stamp: $(HFILE_GLOB) $(CFILE_GLOB)
	@echo 'gtk-doc: Scanning header files'
	gtkdoc-scan --module=$(DOC_MODULE) 		\
	  `for dir in $(DOC_SOURCE_DIR); do		\
	     echo --source-dir=$(srcdir)/$$dir;		\
	   done`					\
	  --ignore-headers="$(IGNORE_HFILES)"		\
	  $(SCAN_OPTIONS) $(EXTRA_HFILES)
	if grep -l '^..*$$' $(srcdir)/$(DOC_MODULE).types > /dev/null 2>&1; then \
	    CC="$(GTKDOC_CC)" LD="$(GTKDOC_LD)"			\
	    CFLAGS="$(GTKDOC_CFLAGS)" LDFLAGS="$(GTKDOC_LIBS)"	\
	    gtkdoc-scangobj $(SCANGOBJ_OPTIONS)			\
	    --module=$(DOC_MODULE) --output-dir=.;		\
	else							\
	    for i in $(SCANOBJ_FILES) ; do			\
               test -f $$i || touch $$i;			\
	    done						\
	fi
	touch scan-build.stamp

$(DOC_MODULE)-decl.txt $(SCANOBJ_FILES) $(DOC_MODULE)-sections.txt $(DOC_MODULE)-overrides.txt: scan-build.stamp
	@true

#### templates ####

tmpl-build.stamp: $(DOC_MODULE)-decl.txt $(SCANOBJ_FILES) $(DOC_MODULE)-sections.txt $(DOC_MODULE)-overrides.txt
	@echo 'gtk-doc: Rebuilding template files'
	gtkdoc-mktmpl --module=$(DOC_MODULE) $(MKTMPL_OPTIONS)
	touch tmpl-build.stamp

tmpl.stamp: tmpl-build.stamp
	@true

tmpl/*.sgml:
	@true


#### xml ####

$(MAIN_SGML_FILE): $(srcdir)/$(DOC_MAIN_SGML_FILE)
	cp $(srcdir)/$(DOC_MAIN_SGML_FILE) $(MAIN_SGML_FILE)

sgml-build.stamp: tmpl.stamp $(HFILE_GLOB) $(CFILE_GLOB) $(MAIN_SGML_FILE) $(DOC_MODULE)-sections.txt $(builddir)/tmpl/*.sgml $(expand_content_files)
	@echo 'gtk-doc: Building XML'
	gtkdoc-mkdb --module=$(DOC_MODULE)			\
	  `for dir in $(DOC_SOURCE_DIR); do			\
	     echo --source-dir=$(srcdir)/$$dir;			\
	   done`						\
	  --output-format=xml					\
	  --expand-content-files="$(expand_content_files)"	\
	  --main-sgml-file=$(MAIN_SGML_FILE)			\
	  $(MKDB_OPTIONS)
	$(EXPAND_RD2_SNIPPET) xml
	touch sgml-build.stamp

sgml.stamp: sgml-build.stamp
	@true

#### html ####

html-build.stamp: sgml.stamp $(CATALOGS) $(MAIN_SGML_FILE) $(content_files) $(HTML_IMAGES)
	@echo 'gtk-doc: Building HTML'
	@echo "English:"
	rm -rf html
	mkdir -p html
	cd html && gtkdoc-mkhtml $(DOC_MODULE) ../$(MAIN_SGML_FILE)
	if test "x$(HTML_IMAGES)" != "x"; then		\
	  for image in $(HTML_IMAGES); do		\
	    if test -r "$(srcdir)/$$image"; then	\
	      cp $(srcdir)/$$image html/;		\
	    else					\
	      cp $(builddir)/$$image html/;		\
	    fi						\
	  done;						\
	fi
	echo 'gtk-doc: Fixing cross-references'
	gtkdoc-fixxref --module-dir=html		\
	  --html-dir=$(HTML_DIR) $(FIXXREF_OPTIONS)
	for catalog in $(CATALOGS); do					\
	  lang=`echo $$catalog | sed 's/.po$$//'`;			\
	  echo "$$lang:";						\
	  rm -rf $$lang;						\
	  mkdir -p $$lang/html;						\
	  mkdir -p $$lang/xml;						\
	  xml2po -k -p $(srcdir)/$$catalog -l $$lang			\
	    $(MAIN_SGML_FILE) > $$lang/$(DOC_MAIN_SGML_FILE);		\
	  for xml in $(srcdir)/xml/*.xml; do				\
	    xml2po -k -p $(srcdir)/$$catalog -l $$lang $$xml >		\
	      $$lang/xml/`basename $$xml`;				\
	  done;								\
	  for file in $(content_files); do				\
	    if test -f $$file; then					\
	      if test -f $$file.$$lang; then				\
	        cp $$file.$$lang $$lang/$$file;				\
	      else							\
	        cp $$file $$lang;					\
	      fi;							\
	    else							\
	      if test -f $(srcdir)/$$file.$$lang; then			\
	        cp $(srcdir)/$$file.$$lang $$lang/$$file;		\
	      else							\
	        cp $(srcdir)/$$file $$lang;				\
	      fi;							\
	    fi;								\
	  done;								\
	  (cd $$lang/html &&						\
	     gtkdoc-mkhtml $(DOC_MODULE) ../$(DOC_MAIN_SGML_FILE));	\
	  sed -i'' -e "s,/,/$$lang/,g" $$lang/html/index.sgml;		\
	  if test "x$(HTML_IMAGES)" != "x"; then			\
	    for image in $(HTML_IMAGES); do				\
	      if test -r "$(srcdir)/$$image"; then			\
	        cp $(srcdir)/$$image $$lang/html/;			\
	      else							\
	        cp $(builddir)/$$image $$lang/html/;			\
	      fi							\
	    done;							\
	  fi;								\
	  echo 'gtk-doc: Fixing cross-references';			\
	  gtkdoc-fixxref --module-dir=$$lang/html			\
	    --html-dir=$(HTML_DIR) $(FIXXREF_OPTIONS);			\
	done
	touch html-build.stamp

##############

clean-local:
	rm -f *~ *.bak
	rm -rf .libs

distclean-local:
	rm -rf xml $(REPORT_FILES) \
	  $(DOC_MODULE)-decl-list.txt $(DOC_MODULE)-decl.txt

maintainer-clean-local: clean
	rm -rf xml html

install-data-local:
	for catalog in '' $(CATALOGS); do				\
	  if test x"$$catalog" = "x"; then				\
	    dir="html";							\
	    target_dir="";						\
	  else								\
	    lang=`echo $$catalog | sed 's/.po$$//'`;			\
	    dir="$$lang/html";						\
	    target_dir="/$$lang";					\
	  fi;								\
	  installfiles=`echo $$dir/*`;					\
	  if test "$$installfiles" = "$$dir/*"; then			\
	    echo '-- Nothing to install';				\
	  else								\
	    $(mkinstalldirs) $(DESTDIR)$(TARGET_DIR)$$target_dir;	\
	    for i in $$installfiles; do					\
	      echo "-- Installing $$i";					\
	      $(INSTALL_DATA) $$i $(DESTDIR)$(TARGET_DIR)$$target_dir;	\
	    done;							\
	    echo "-- Installing $$dir/index.sgml";			\
	    $(INSTALL_DATA) $$dir/index.sgml				\
	      $(DESTDIR)$(TARGET_DIR)$$target_dir || :;			\
	    if test "$(DESTDIR)" = ""; then				\
	      $(GTKDOC_REBASE) --relative 				\
	        --html-dir=$(DESTDIR)$(TARGET_DIR)$$target_dir;		\
	    else							\
	      $(GTKDOC_REBASE) --relative --dest-dir=$(DESTDIR)		\
	        --html-dir=$(DESTDIR)$(TARGET_DIR)$$target_dir;		\
	    fi;								\
	  fi;								\
	done

uninstall-local:
	rm -rf $(DESTDIR)$(TARGET_DIR)/*

#
# Require gtk-doc when making dist
#
if ENABLE_GTK_DOC
dist-check-gtkdoc:
else
dist-check-gtkdoc:
	@echo "*** gtk-doc must be installed and enabled in order to make dist"
	@false
endif

dist-hook: dist-check-gtkdoc dist-hook-local
	mkdir $(distdir)/tmpl
	mkdir $(distdir)/xml
	mkdir $(distdir)/html
	-cp tmpl/*.sgml $(distdir)/tmpl
	-cp xml/*.xml $(distdir)/xml
	cp html/* $(distdir)/html
	for catalog in $(CATALOGS); do					\
	  lang=`echo $$catalog | sed 's/.po$$//'`;			\
	  mkdir -p $(distdir)/$$lang/html;				\
	  mkdir -p $(distdir)/$$lang/xml;				\
	  cp $$lang/html/* $(distdir)/$$lang/html;			\
	  cp $$lang/xml/* $(distdir)/$$lang/html;			\
	  cp $$lang/$(DOC_MAIN_SGML_FILE) $(distdir)/$$lang/;		\
	done
	cp $(DOC_MODULE).types $(distdir)/
	cp $(DOC_MODULE)-sections.txt $(distdir)/
	cd $(distdir) && rm -f $(DISTCLEANFILES)
	-$(GTKDOC_REBASE) --online --relative --html-dir=$(distdir)/html

.PHONY : dist-hook-local docs
