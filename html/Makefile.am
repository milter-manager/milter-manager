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
	introduction.svg			\
	favicon.svg

images =					\
	famfamfam-logo.png			\
	tango-logo.png				\
	jp.png					\
	us.png					\
	$(generated_images)

generated_images =				\
	bar.png					\
	blog.png				\
	download.png				\
	heading-mark.png			\
	install.png				\
	mini-install.png			\
	introduction.png			\
	logo.png				\
	what-is-milter-manager.png		\
	favicon.png				\
	favicon.ico

MAINTAINER_CLEANFILES = $(generated_images)

upload: $(web_contents)
	(cd $(srcdir) && rsync -avz $(web_contents) $(SF_HTDOCS)/)

SUFFIXES = .svg .png

.svg.png:
	inkscape --export-png $@ $<

mini-install.png: install.png
	convert install.png -resize 12x12 mini-install.png
