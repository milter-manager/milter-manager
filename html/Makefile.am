EXTRA_DIST =					\
	$(web_contents)				\
	$(svg_images)

web_contents =					\
	index.html				\
	index.html.ja				\
	milter-manager.css			\
	$(images)

svg_images =					\
	logo.svg				\
	what-is-milter-manager.svg		\
	bar.svg					\
	blog.svg				\
	download.svg				\
	heading-mark.svg			\
	install.svg				\
	introduction.svg

images =					\
	famfamfam-logo.png			\
	tango-logo.png				\
	jp.png					\
	us.png					\
	$(generated_images)

auto_generated_images =				\
	logo.png				\
	what-is-milter-manager.png

generated_images =				\
	bar.png					\
	blog.png				\
	download.png				\
	heading-mark.png			\
	install.png				\
	introduction.png			\
	$(auto_generated_images)

MAINTAINER_CLEANFILES = $(auto_generated_images)

upload: $(web_contents)
	for file in $(web_contents); do				\
	  scp $(srcdir)/$$file $(SF_HTDOCS)/;			\
	done

SUFFIXES = .svg .png

.svg.png:
	inkscape --export-background=white --export-png $@ $<
