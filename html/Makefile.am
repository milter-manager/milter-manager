EXTRA_DIST =					\
	$(web_contents)				\
	logo.svg				\
	what-is-milter-manager.svg

web_contents =					\
	index.html				\
	index.html.ja				\
	milter-manager.css			\
	$(generated_images)

generated_images =				\
	logo.png				\
	what-is-milter-manager.png

MAINTAINER_CLEANFILES = $(generated_images)

upload: $(web_contents)
	for file in $(web_contents); do				\
	  scp $(srcdir)/$$file $(SF_HTDOCS)/;			\
	done

SUFFIXES = .svg .png

.svg.png:
	inkscape --export-background=white --export-png $@ $<
