CODE_NAMES = trusty,wily,xenial
SOURCE = ../$(PACKAGE)-$(VERSION).tar.gz

all:

ensure-launchpad-configuration:
	@if test -z "$(LAUNCHPAD_UPLOADER_PGP_KEY)"; then		\
	  echo "--with-launchpad-uploader-pgp-key configure option must be specified.";	\
	  false;							\
	fi

upload: source ensure-launchpad-configuration
	./upload.rb \
	  --package '$(PACKAGE)' \
	  --version '$(VERSION)' \
	  --source-archive '$(SOURCE)' \
	  --code-names '$(CODE_NAMES)' \
	  --debian-directory '$(srcdir)/../debian/' \
	  --pgp-sign-key '$(LAUNCHPAD_UPLOADER_PGP_KEY)'

upload: upload-precise
upload-precise: source ensure-launchpad-configuration
	rm -rf precise
	mkdir -p precise
	cp -r '$(srcdir)/../debian/' precise/
	sed -i \
	  -e 's/ruby (>= 1:1.9.3)/ruby1.9.1 (>= 1.9.1)/g' \
	  -e 's/ruby-dev (>= 1:1.9.3)/ruby1.9.1-dev (>= 1.9.1)/g' \
	  -e '/ruby-gnome2-dev/d' \
	  -e '/ruby-glib2/d' \
	  -e '/dh-systemd/d' \
	  precise/debian/control
	sed -i \
	  -e '/--enable-ruby-milter/i \\t\t--with-ruby=/usr/bin/ruby1.9.1 \\' \
	  -e 's/ruby -rrbconfig/ruby1.9.1 -rrbconfig/g' \
	  -e 's/ --with=systemd//' \
	  precise/debian/rules
	sed -i -e 's,^# ,,' precise/debian/ruby-milter-core.install
	./upload.rb \
	  --package '$(PACKAGE)' \
	  --version '$(VERSION)' \
	  --source-archive '$(SOURCE)' \
	  --code-names 'precise' \
	  --debian-directory 'precise/debian' \
	  --pgp-sign-key '$(LAUNCHPAD_UPLOADER_PGP_KEY)'

source: $(SOURCE)

$(SOURCE):
	ln -s $(abs_top_builddir)/$(PACKAGE)-$(VERSION).tar.gz $(SOURCE)
