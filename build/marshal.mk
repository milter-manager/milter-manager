# -*- Mode: Makefile; tab-width: 8; indent-tabs-mode: t; -*-

marshal_sources =			\
	$(marshal_source_prefix).c	\
	$(marshal_source_prefix).h

EXTRA_DIST += $(marshal_source_prefix).list
BUILT_SOURCES += $(marshal_sources)
MAINTAINERCLEANFILES +=	$(marshal_sources)

$(marshal_source_prefix).h: $(marshal_source_prefix).list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) $(srcdir)/$(marshal_source_prefix).list --header --prefix=$(marshal_prefix) > $@

$(marshal_source_prefix).c: $(marshal_source_prefix).list
	$(AM_V_GEN) $(GLIB_GENMARSHAL) $(srcdir)/$(marshal_source_prefix).list --header --body --prefix=$(marshal_prefix) > $@
