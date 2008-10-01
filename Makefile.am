SUBDIRS =		\
	milter-core	\
	milter-client	\
	milter-server	\
	libmilter	\
	test

EXTRA_DIST =		\
	autogen.sh

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
	  (find milter-client -name '*.c') | \
	  sort > po/POTFILES.in
	cd po && $(MAKE) $(AM_MAKEFLAGS) update-po

tag:
	svn ls $(SF_REPOS)/tags/$(VERSION) > /dev/null 2>&1; 	\
	  if test "$$?" = "0"; then				\
	    echo "$(VERSION) is already tagged";		\
	    exit 1;						\
	  fi
	svn cp -m "released $(VERSION)!!!" $(SF_USER_OPTION) \
	  $(SF_REPOS)/trunk $(SF_REPOS)/tags/$(VERSION)
