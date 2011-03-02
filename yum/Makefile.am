SERVER_PATH = $(SF_USER),$(SF_PROJECT_ID)@$(SF_HTDOCS)
# DISTRIBUTIONS = fedora centos
DISTRIBUTIONS = centos
ARCHITECTURES = i386 x86_64
CHROOT_BASE = /var/lib/chroot
HAVE_DEVELOPMENT_BRANCH = yes

release: remove-existing-packages build sign update upload

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
	  rsync -avz						\
	    $(SERVER_PATH)/$${distribution}/ $${distribution};	\
	done

build: build-in-chroot build-repository-rpm

build-in-chroot:
	./build-in-chroot.sh \
	  $(PACKAGE) $(VERSION) $(CHROOT_BASE) \
	  '$(ARCHITECTURES)' '$(DISTRIBUTIONS)'

build-repository-rpm: RPM-GPG-KEY-milter-manager
	./build-repository-rpm.sh \
	  $(PACKAGE) '$(PACKAGE_TITLE)' \
	  http://$(PACKAGE).sourceforge.net \
	  '$(DISTRIBUTIONS)' \
	  $(HAVE_DEVELOPMENT_BRANCH)

RPM-GPG-KEY-milter-manager:
	./gpg-public-key.sh > $@
