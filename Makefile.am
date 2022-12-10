ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_OPTIONS}

SUBDIRS =					\
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
	package					\
	vendor

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
	NEWS					\
	NEWS.ja					\
	README					\
	README.ja				\
	autogen.sh				\
	gpg_uid					\
	meson-config.h.in			\
	meson.build				\
	meson_options.txt

AM_DISTCHECK_CONFIGURE_FLAGS =			\
	--enable-gtk-doc

OSDN_CREDENTIAL_FILE = $(HOME)/.config/osdn/credential.yml

$(PACKAGE)-$(VERSION).tar.gz:
	wget https://github.com/milter-manager/milter-manager/releases/download/$(VERSION)/$(PACKAGE)-$(VERSION).tar.gz

upload-sign: $(PACKAGE)-$(VERSION).tar.gz.asc
	gh release upload			\
	  --repo milter-manager/milter-manager	\
	  $(VERSION)				\
	  $(PACKAGE)-$(VERSION).tar.gz.asc

sign-archive: $(PACKAGE)-$(VERSION).tar.gz
	gpg --local-user $(GPG_UID) --armor --detach-sign $(PACKAGE)-$(VERSION).tar.gz

# Deprecated. Use update-doc instead.
upload-doc:
	cd html && $(MAKE) $(AM_MAKEFLAGS) upload
	cd doc/reference && $(MAKE) $(AM_MAKEFLAGS) upload

update-doc:
	cd $(srcdir);						\
	source_reference="$$(git describe)";			\
	case "$${source_reference}" in				\
	  *-*)							\
	    source_reference=master;				\
	    ;;							\
	esac;							\
	gh workflow						\
	  --repo milter-manager/milter-manager.github.io	\
	  run							\
	  site.yml						\
	  --raw-field "source-reference=$${source_reference}"

upload-coverage: coverage-force
	rsync -avz coverage/ $(OSDN_HTDOCS)/coverage/

update-po:
	cd $(top_srcdir) &&				\
	  (find milter -name '*.c';			\
	   find module -name '*.c';			\
	   find tool -name '*.c') |			\
	  sort > po/POTFILES.in
	cd po && $(MAKE) $(AM_MAKEFLAGS) update-po

update-files:
	cd test/fixtures && $(MAKE) $(AM_MAKEFLAGS) update-files

update-latest-release:
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
	  $(CUTTER_SOURCE_PATH)/misc/update-latest-release.rb	\
	  $(PACKAGE)						\
	  $(OLD_RELEASE) $(OLD_RELEASE_DATE)			\
	  $(VERSION) $(NEW_RELEASE_DATE)			\
	  package/debian/changelog				\
	  package/yum/milter-manager.spec.in			\
	  html/index.html					\
	  html/index.html.ja					\
	  html/blog/tdiary.en.conf				\
	  html/blog/tdiary.ja.conf				\
	  doc/reference/header.html				\
	  doc/reference/header-ja.html				\
	  doc/install-to-centos.rd				\
	  doc/install-to-centos.rd.ja				\
	  doc/install-to-freebsd.rd				\
	  doc/install-to-freebsd.rd.ja				\
	  doc/upgrade-on-centos.rd				\
	  doc/upgrade-on-centos.rd.ja				\
	  doc/upgrade-on-freebsd.rd				\
	  doc/upgrade-on-freebsd.rd.ja				\
	  site/_config.yml

tag:
	cd $(srcdir) && git tag -a "$(VERSION)" -m "released $(VERSION)!!!"

echo-abs-top-srcdir:
	@echo $(abs_top_srcdir)

echo-abs-top-builddir:
	@echo $(abs_top_builddir)

echo-cutter:
	@echo $(CUTTER)

echo-ruby:
	@echo $(RUBY)
