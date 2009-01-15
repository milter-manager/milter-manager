EXTRA_DIST = index.html index.html.ja

upload:
	scp $(srcdir)/index.html $(srcdir)/index.html.ja $(SF_HTDOCS)/
	ssh $(SF_HOST) chmod g+w $(SF_HTDOCS_PATH)/index.html
	ssh $(SF_HOST) chmod g+w $(SF_HTDOCS_PATH)/index.html.ja
