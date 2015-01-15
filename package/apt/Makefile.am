SERVER_PATH = $(SF_FRS)
DISTRIBUTIONS = debian ubuntu
ARCHITECTURES = i386 amd64
CODE_NAMES = wheezy jessie unstable lucid precise trusty utopic
CLEAN_ROOM_BASE = /var/cache/pbuilder

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
	./build-in-clean-room.sh \
	  $(VERSION) $(CLEAN_ROOM_BASE) \
	  '$(ARCHITECTURES)' '$(CODE_NAMES)'; \
