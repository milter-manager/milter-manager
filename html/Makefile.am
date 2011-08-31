EXTRA_DIST =					\
	$(web_contents)				\
	$(svg_images)

web_contents =					\
	.htaccess				\
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
	upgrade.svg				\
	introduction.svg			\
	favicon.svg

images =					\
	famfamfam-logo.png			\
	tango-logo.png				\
	github-logo.png				\
	jp.png					\
	us.png					\
	reference.png				\
	$(generated_images)

generated_images =				\
	bar.png					\
	blog.png				\
	download.png				\
	heading-mark.png			\
	mini-heading-mark.png			\
	install.png				\
	mini-install.png			\
	upgrade.png				\
	mini-upgrade.png			\
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

mini-upgrade.png: upgrade.png
	convert upgrade.png -resize 12x12 mini-upgrade.png

mini-heading-mark.png: heading-mark.png
	convert heading-mark.png -resize 19x19 mini-heading-mark.png
