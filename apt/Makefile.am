SERVER_PATH = $(SF_USER),$(SF_PROJECT_ID)@$(SF_HTDOCS)
DISTRIBUTIONS = debian ubuntu
CHROOT_BASE = /var/lib/chroot
ARCHITECTURES = i386 amd64
CODE_NAMES = lenny squeeze unstable hardy lucid maverick

release: build update sign upload

dist:
	cd ${top_builddir} && $(MAKE) dist

remove-existing-packages:
	for distribution in $(DISTRIBUTIONS); do	\
	  find $${distribution}/pool -not		\
	    -path '*/.svn/*' \-type f -delete;		\
	done

update:
	./update-repository.sh \
	  '$(SF_PROJECT_NAME)' '$(PACKAGE_TITLE)' '$(CODE_NAMES)'

sign:
	./sign-repository.sh '$(CODE_NAMES)'

upload:
	for distribution in $(DISTRIBUTIONS); do		\
	  (cd $${distribution} &&				\
	   rsync -avz --exclude .svn --delete			\
	     .htaccess dists pool stable development	\
	     $(SERVER_PATH)/$${distribution};			\
	  );							\
	done

download:
	for distribution in $(DISTRIBUTIONS); do			\
	  (mkdir -p $${distribution} &&					\
	   cd $${distribution} &&					\
	   rm -rf pool dists &&						\
	   ln -fs stable/pool pool &&					\
	   ln -fs stable/dists dists &&					\
	   rsync -avz $(SERVER_PATH)/$${distribution}/stable/		\
	     stable &&							\
	   rsync -avz $(SERVER_PATH)/$${distribution}/development/	\
	     development;						\
	  );								\
	done

build:
	cp $(top_srcdir)/data/init.d/debian/milter-manager \
	  $(top_srcdir)/debian/milter-manager.init
	cp $(top_srcdir)/data/init.d/debian/default/milter-manager \
	  $(top_srcdir)/debian/milter-manager.default
	cp $(top_srcdir)/data/cron.d/debian/milter-manager-log \
	  $(top_srcdir)/debian/milter-manager.cron.d
	./build-in-chroot.sh \
	  $(VERSION) $(CHROOT_BASE) \
	  '$(ARCHITECTURES)' '$(CODE_NAMES)'
