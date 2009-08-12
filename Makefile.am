SUBDIRS =		\
	milter		\
	libmilter	\
	binding		\
	module		\
	src		\
	tool		\
	data		\
	admin		\
	test		\
	po		\
	build		\
	misc		\
	doc		\
	html		\
	license

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA =				\
	milter-core.pc				\
	milter-client.pc			\
	milter-server.pc			\
	milter-manager.pc			\
	libmilter.pc

EXTRA_DIST =					\
	autogen.sh				\
	ChangeLog.toolkit			\
	README					\
	README.ja				\
	NEWS					\
	NEWS.ja					\
	milter-manager.spec

upload: upload-doc upload-coverage

release: dist
	$(srcdir)/misc/release.rb				\
	  $(SF_USER) '$(SF_PROJECT_NAME)' '$(PACKAGE_TITLE)'	\
	  $(VERSION) README NEWS				\
	  $(PACKAGE)-$(VERSION).tar.gz				\
	  $(PACKAGE)-$(VERSION)-0.src.rpm			\
	  $(PACKAGE)-$(VERSION)-0.i386.rpm			\
	  $(PACKAGE)-$(VERSION)-0.x86_64.rpm

upload-doc:
	cd html && $(MAKE) $(AM_MAKEFLAGS) upload
	cd doc/reference && $(MAKE) $(AM_MAKEFLAGS) upload

upload-coverage: coverage
	rsync -avz coverage/ $(SF_HTDOCS)/coverage/

update-po:
	cd $(top_srcdir) &&				\
	  (find milter -name '*.c';			\
	   find module -name '*.c';			\
	   find tool -name '*.c') |			\
	  sort > po/POTFILES.in
	cd po && $(MAKE) $(AM_MAKEFLAGS) update-po

tag:
	svn ls $(SF_REPOS)/tags/$(VERSION) > /dev/null 2>&1; 	\
	  if test "$$?" = "0"; then				\
	    echo "$(VERSION) is already tagged";		\
	    exit 1;						\
	  fi
	svn cp -m "released $(VERSION)!!!" $(SF_USER_OPTION)	\
	  $(SF_REPOS)/trunk $(SF_REPOS)/tags/$(VERSION)
