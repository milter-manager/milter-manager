SUBDIRS =		\
	milter		\
	libmilter	\
	binding		\
	module		\
	src		\
	tool		\
	data		\
	test		\
	doc

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
	INTRODUCTION				\
	INTRODUCTION.ja

upload: upload-doc

release: dist
	$(srcdir)/misc/release.rb				\
	  $(SF_USER) $(SF_PROJECT_NAME) $(PACKAGE_TITLE)	\
	  $(VERSION) $(PACKAGE)-$(VERSION).tar.gz       	\
	  README NEWS

upload-doc:
	cd html && $(MAKE) $(AM_MAKEFLAGS) upload
	cd doc/reference && $(MAKE) $(AM_MAKEFLAGS) upload

update-po:
	cd $(top_srcdir) && \
	  (find milter -name '*.c') | \
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
