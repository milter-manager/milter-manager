EXTRA_DIST =					\
	index.html				\
	index.html.ja				\
	milter-manager.css			\
	logo.svg				\
	what-is-milter-manager.svg		\
	$(generated_images)

generated_images =				\
	logo.png				\
	what-is-milter-manager.png

MAINTAINER_CLEANFILES = $(generated_images)

upload: $(generated_images)
	for file in $(EXTRA_DIST); do				\
	  scp $(srcdir)/$$file $(SF_HTDOCS)/;			\
	  ssh $(SF_HOST) chmod g+w $(SF_HTDOCS_PATH)/$$file;	\
	done

SUFFIXES = .svg .png

.svg.png:
	inkscape --export-background=white --export-png $@ $<
