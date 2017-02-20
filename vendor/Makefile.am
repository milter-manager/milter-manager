SUBDIRS =

dist-hook:
	mkdir -p $(distdir)/cutter-source/misc
	find $(srcdir)/cutter-source -path '*misc/*.rb' -a -type f |\
	  xargs cp -a -t $(distdir)/cutter-source/misc/
