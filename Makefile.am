ACLOCAL_AMFLAGS = $$ACLOCAL_OPTIONS

SUBDIRS =					\
	libev-4.04				\
	milter					\
	libmilter				\
	binding					\
	module					\
	src					\
	tool					\
	data					\
	test					\
	po					\
	build					\
	doc					\
	html					\
	license					\
	package

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA =				\
	milter-core.pc				\
	milter-client.pc			\
	milter-server.pc			\
	milter-manager.pc			\
	libmilter.pc

if WITH_RUBY_MILTER
pkgconfig_DATA +=				\
	ruby-milter.pc				\
	ruby-milter-core.pc			\
	ruby-milter-client.pc			\
	ruby-milter-server.pc
endif

EXTRA_DIST =					\
	autogen.sh				\
	README					\
	README.ja				\
	NEWS					\
	NEWS.ja

upload: upload-doc upload-coverage

release: dist
	$(srcdir)/misc/release.rb				\
	  $(SF_USER) '$(SF_PROJECT_ID)' '$(SF_PROJECT_NAME)'	\
	  '$(PACKAGE_TITLE)' $(VERSION) README NEWS		\
	  $(PACKAGE)-$(VERSION).tar.gz

upload-doc:
	cd html && $(MAKE) $(AM_MAKEFLAGS) upload
	cd doc/reference && $(MAKE) $(AM_MAKEFLAGS) upload

upload-coverage: coverage-force
	rsync -avz coverage/ $(SF_HTDOCS)/coverage/

update-po:
	cd $(top_srcdir) &&				\
	  (find milter -name '*.c';			\
	   find module -name '*.c';			\
	   find tool -name '*.c') |			\
	  sort > po/POTFILES.in
	cd po && $(MAKE) $(AM_MAKEFLAGS) update-po

update-files:
	cd test/fixtures && $(MAKE) $(AM_MAKEFLAGS) update-files

update-latest-release: misc
	@if test -z "$(OLD_RELEASE)"; then	\
	  echo "\$$(OLD_RELEASE) is missing";	\
	  exit 1;				\
	fi
	@if test -z "$(OLD_RELEASE_DATE)"; then		\
	  echo "\$$(OLD_RELEASE_DATE) is missing";	\
	  exit 1;					\
	fi
	@if test -z "$(NEW_RELEASE_DATE)"; then		\
	  echo "\$$(NEW_RELEASE_DATE) is missing";	\
	  exit 1;					\
	fi
	cd $(top_srcdir) &&					\
	  misc/update-latest-release.rb				\
	  $(PACKAGE)						\
	  $(OLD_RELEASE) $(OLD_RELEASE_DATE)			\
	  $(VERSION) $(NEW_RELEASE_DATE)			\
	  package/rpm/centos/milter-manager.spec.in		\
	  package/debian/changelog				\
	  html/index.html					\
	  html/index.html.ja					\
	  html/blog/tdiary.conf					\
	  doc/reference/header$(RELEASE_TYPE_SUFFIX).html	\
	  doc/reference/header$(RELEASE_TYPE_SUFFIX)-ja.html	\
	  doc/install-to-centos.rd				\
	  doc/install-to-centos.rd.ja				\
	  doc/install-to-freebsd.rd				\
	  doc/install-to-freebsd.rd.ja				\
	  doc/upgrade-on-centos.rd				\
	  doc/upgrade-on-centos.rd.ja				\
	  doc/upgrade-on-freebsd.rd				\
	  doc/upgrade-on-freebsd.rd.ja

tag:
	git tag -a "$(VERSION)" -m "released $(VERSION)!!!"

misc:
	@if test -z "$(CUTTER_SOURCE_PATH)"; then		\
	  echo "\$$(CUTTER_SOURCE_PATH) is missing";		\
	  exit 1;						\
	fi
	ln -s "$(CUTTER_SOURCE_PATH)/misc" misc;

echo-abs-top-srcdir:
	@echo $(abs_top_srcdir)

echo-abs-top-builddir:
	@echo $(abs_top_builddir)

echo-cutter:
	@echo $(CUTTER)

echo-ruby:
	@echo $(RUBY)
