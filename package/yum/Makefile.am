SERVER_PATH = $(SF_FRS)
# DISTRIBUTIONS = fedora centos
DISTRIBUTIONS = centos
ARCHITECTURES = i386 x86_64
CHROOT_BASE = /var/lib/chroot
BASE_URL_PREFIX = http://downloads.sourceforge.net/project/$(PACKAGE)
HAVE_DEVELOPMENT_BRANCH = yes
USE_RPMFORGE = no
USE_ATRPMS = no
USE_EPEL = yes

release: build sign update upload

dist:
	cd ${top_builddir} && $(MAKE) dist

remove-existing-packages:
	for distribution in $(DISTRIBUTIONS); do	\
	  find $${distribution} -not -path '*/.svn/*'	\
	     -name "*.rpm" -delete;			\
	done

sign:
	./sign-rpm.sh '$(DISTRIBUTIONS)'

update:
	./update-repository.sh $(PACKAGE) '$(DISTRIBUTIONS)'

upload:
	for distribution in $(DISTRIBUTIONS); do		\
	  rsync -avz --delete					\
	    --exclude .gitignore --exclude .svn			\
	    $${distribution}/ $(SERVER_PATH)/$${distribution};	\
	done

download:
	for distribution in $(DISTRIBUTIONS); do		\
	  rsync -avz --delete						\
	    $(SERVER_PATH)/$${distribution}/ $${distribution};	\
	done

build: build-in-chroot build-release-rpm

build-in-chroot:
	./build-in-chroot.sh			\
	  $(PACKAGE)				\
	  $(VERSION)				\
	  $(top_builddir)/$(PACKAGE)		\
	  $(top_builddir)/package/rpm			\
	  $(CHROOT_BASE)			\
	  '$(ARCHITECTURES)'			\
	  '$(DISTRIBUTIONS)'			\
	  '$(HAVE_DEVELOPMENT_BRANCH)'		\
	  '$(USE_RPMFORGE)'			\
	  '$(USE_ATRPMS)'			\
	  '$(USE_EPEL)'

build-release-rpm: RPM-GPG-KEY-$(PACKAGE)
	./build-release-rpm.sh		\
	  $(PACKAGE)				\
	  '$(PACKAGE_TITLE)'			\
	  $(BASE_URL_PREFIX)			\
	  '$(DISTRIBUTIONS)'			\
	  $(HAVE_DEVELOPMENT_BRANCH)

RPM-GPG-KEY-$(PACKAGE):
	./gpg-public-key.sh > $@
