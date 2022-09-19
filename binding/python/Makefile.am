EXTRA_DIST =					\
	meson.build

dist-hook:
	cp -a $(srcdir)/gi $(distdir)/
	cp -a $(srcdir)/milter $(distdir)/
	cp -a $(srcdir)/sample $(distdir)/
	find $(distdir) -type d -name __pycache__ -exec rm '{}' ';'
