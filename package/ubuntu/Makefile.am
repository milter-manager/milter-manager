UBUNTU_CODE_NAMES = bionic,focal,impish
UBUNTU_VERSIONS = 18.04,20.04,21.10
PPA = milter-manager-ppa
PPA_INCOMING = ~milter-manager/ppa/ubuntu/
SOURCE = ../$(PACKAGE)-$(VERSION).tar.gz

all:

ensure-launchpad-configuration:
	@if test -z "$(LAUNCHPAD_UPLOADER_PGP_KEY)"; then		\
	  echo "--with-launchpad-uploader-pgp-key configure option must be specified.";	\
	  false;							\
	fi

upload: source ensure-launchpad-configuration
	rm -rf debian
	cp -a '$(srcdir)/../debian' debian
	sed -i -e '/cutter/d' debian/control
	sed -i -e '/TZ=Asia/d' debian/rules
	$(GROONGA_SOURCE_PATH)/packages/ubuntu/upload.rb \
	  --ppa '$(PPA)' \
	  --ppa-incoming '$(PPA_INCOMING)' \
	  --package '$(PACKAGE)' \
	  --version '$(VERSION)' \
	  --source-archive '$(SOURCE)' \
	  --ubuntu-code-names '$(UBUNTU_CODE_NAMES)' \
	  --ubuntu-versions '$(UBUNTU_VERSIONS)' \
	  --debian-directory 'debian' \
	  --pgp-sign-key '$(LAUNCHPAD_UPLOADER_PGP_KEY)'

source: $(SOURCE)

$(SOURCE):
	ln -s $(abs_top_builddir)/$(PACKAGE)-$(VERSION).tar.gz $(SOURCE)
