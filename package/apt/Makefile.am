DISTRIBUTIONS = debian
ARCHITECTURES = i386 amd64
CODE_NAMES = buster bullseye unstable
CLEAN_ROOM_BASE = /var/cache/pbuilder
RELEASE = 1

release: build upload

dist:
	cd ${top_builddir} && $(MAKE) dist

remove-existing-packages:
	for distribution in $(DISTRIBUTIONS); do	\
	  find $${distribution}/pool -not		\
	    -path '*/.svn/*' \-type f -delete;		\
	done

update:
	$(srcdir)/update-repository.sh \
	  '$(OSDN_PROJECT_NAME)' '$(PACKAGE_TITLE)' '$(CODE_NAMES)'

sign:
	$(srcdir)/sign-repository.sh $(GPG_UID) '$(CODE_NAMES)'

upload:
	$(srcdir)/upload-packages.sh					\
	  $(OSDN_PROJECT_ID) $(VERSION) $(RELEASE) '$(ARCHITECTURES)' '$(CODE_NAMES)'

build:
	$(srcdir)/build-in-clean-room.sh \
	  $(VERSION) $(CLEAN_ROOM_BASE) \
	  '$(ARCHITECTURES)' '$(CODE_NAMES)'
