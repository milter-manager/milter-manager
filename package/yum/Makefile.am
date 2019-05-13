# DISTRIBUTIONS = fedora centos
DISTRIBUTIONS = centos
CENTOS_VERSIONS= 7
ARCHITECTURES = x86_64
CHROOT_BASE = /var/lib/chroot
BASE_URL_PREFIX = http://downloads.sourceforge.net/project/$(PACKAGE)
USE_RPMFORGE = no
USE_ATRPMS = no
USE_EPEL = yes
RELEASE = 1

release: build upload

dist:
	cd ${top_builddir} && $(MAKE) dist

remove-existing-packages:
	for distribution in $(DISTRIBUTIONS); do	\
	  find $${distribution} -not -path '*/.svn/*'	\
	     -name "*.rpm" -delete;			\
	done

sign:
	./sign-rpm.sh $(GPG_UID) '$(DISTRIBUTIONS)'

update:
	./update-repository.sh $(PACKAGE) '$(DISTRIBUTIONS)'

upload:
	for distribution in $(DISTRIBUTIONS); do		\
	  ./upload-packages.sh $${distribution} '$(CENTOS_VERSIONS)' $(OSDN_PROJECT_ID) $(VERSION) $(RELEASE); \
	done

build: build-in-chroot

build-in-chroot:
	./build-in-chroot.sh			\
	  $(PACKAGE)				\
	  $(VERSION)				\
	  $(top_builddir)/$(PACKAGE)		\
	  $(top_builddir)/package/rpm			\
	  $(CHROOT_BASE)			\
	  '$(ARCHITECTURES)'			\
	  '$(DISTRIBUTIONS)'			\
	  '$(USE_RPMFORGE)'			\
	  '$(USE_ATRPMS)'			\
	  '$(USE_EPEL)'

ensure-public-key:
	gpg --list-keys '$(GPG_UID)' > /dev/null ||			\
	  gpg --keyserver keyserver.ubuntu.com --recv-key '$(GPG_UID)'

RPM-GPG-KEY-$(PACKAGE): ensure-public-key
	gpg --armor --export '$(GPG_UID)' > $@
