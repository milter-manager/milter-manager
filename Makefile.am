ACLOCAL_AMFLAGS = $$ACLOCAL_OPTIONS

SUBDIRS =

if WITH_BUNDLED_LIBEV
SUBDIRS +=					\
	libev-4.19
endif

SUBDIRS +=					\
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
	gpg_uid					\
	README					\
	README.ja				\
	NEWS					\
	NEWS.ja

OSDN_CREDENTIAL_FILE = $(HOME)/.config/osdn/credential.yml

upload: upload-doc upload-coverage

release: release-osdn

release-github: sign-archive
	$(CUTTER_SOURCE_PATH)/misc/release-to-github.rb \
	  --repository=milter-manager/milter-manager \
	  --tag=$(VERSION) \
	  --news-file=NEWS \
	  --asset-file=$(PACKAGE)-$(VERSION).tar.gz \
	  --asset-file=$(PACKAGE)-$(VERSION).tar.gz.asc
	  --access-token-file=$(ACCESS_TOKEN_FILE)

release-osdn: sign-archive
	$(CUTTER_SOURCE_PATH)/misc/release-to-osdn.rb \
	  --credential=$(OSDN_CREDENTIAL_FILE) \
	  --project=$(PACKAGE) \
	  --new-version=$(VERSION) \
	  --news-file=NEWS \
	  --package=$(PACKAGE) \
	  --asset-file=$(PACKAGE)-$(VERSION).tar.gz \
	  --asset-file=$(PACKAGE)-$(VERSION).tar.gz.asc

sign-archive: dist
	gpg2 --local-user $(GPG_UID) --armor --detach-sign $(PACKAGE)-$(VERSION).tar.gz

upload-doc:
	cd html && $(MAKE) $(AM_MAKEFLAGS) upload
	cd doc/reference && $(MAKE) $(AM_MAKEFLAGS) upload

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
	  package/rpm/centos/milter-manager.spec.in		\
	  package/debian/changelog				\
	  html/index.html					\
	  html/index.html.ja					\
	  html/blog/tdiary.conf					\
	  doc/reference/header.html	\
	  doc/reference/header-ja.html	\
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

echo-abs-top-srcdir:
	@echo $(abs_top_srcdir)

echo-abs-top-builddir:
	@echo $(abs_top_builddir)

echo-cutter:
	@echo $(CUTTER)

echo-ruby:
	@echo $(RUBY)
