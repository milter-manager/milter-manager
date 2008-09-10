INCLUDES = 			\
	 -I$(top_builddir)	\
	 -I$(top_srcdir)

AM_CFLAGS = 				\
	-DLOCALEDIR=\""$(localedir)"\"	\
	$(GLIB_CFLAGS)			\
	$(COVERAGE_CFLAGS)

EXTRA_DIST = mc-marshalers.list

milter_client_public_headers =		\
	mc-parser.h

enum_source_prefix = mc-enum-types

enum_sources_h =			\
	$(milter_client_public_headers)

enum_sources =			\
	$(enum_source_prefix).c	\
	$(enum_source_prefix).h

marshal_sources =		\
	mc-marshalers.c		\
	mc-marshalers.h

MAINTAINERCLEANFILES =			\
	stamp-$(enum_source_prefix)-c	\
	stamp-$(enum_source_prefix)-h   \
	$(marshal_sources)

BUILT_SOURCES = $(enum_sources) $(marshal_sources)

milter_client_includedir=$(pkgincludedir)/milter-client
milter_client_include_HEADERS =		\
	mc-enum-types.h			\
	mc-marshalers.h			\
	$(milter_client_public_headers)

lib_LTLIBRARIES =		\
	libmilter-client.la

libmilter_client_la_SOURCES =		\
	mc-enum-types.c			\
	mc-marshalers.c			\
	mc-parser.c


$(enum_source_prefix).c: stamp-$(enum_source_prefix)-c $(enum_source_prefix).h
	@true

stamp-$(enum_source_prefix)-c: $(enum_sources_h) Makefile
	(cd $(srcdir) && \
	  include_headers="" && \
	  for h in $(enum_sources_h); do \
	    include_headers="$${include_headers}#include \"$${h}\"\n"; \
	  done && \
	  $(GLIB_MKENUMS) \
	    --fhead "#include \"$(enum_source_prefix).h\"\n$${include_headers}" \
	    --fprod "\n/* enumerations from \"@filename@\" */" \
	    --vhead "GType\n@enum_name@_get_type (void)\n{\n  static GType etype = 0;\n  if (etype == 0) {\n    static const G@Type@Value values[] = {" 	\
	    --vprod "      { @VALUENAME@, \"@VALUENAME@\", \"@valuenick@\" }," \
	    --vtail "      { 0, NULL, NULL }\n    };\n    etype = g_@type@_register_static (\"@EnumName@\", values);\n  }\n  return etype;\n}\n" \
	    $(enum_sources_h)) > tmp-$(enum_source_prefix).c && \
	(cmp -s tmp-$(enum_source_prefix).c $(enum_source_prefix).c || \
	  cp tmp-$(enum_source_prefix).c $(enum_source_prefix).c ) && \
	rm -f tmp-$(enum_source_prefix).c && \
	echo timestamp > $(@F)

$(enum_source_prefix).h: stamp-$(enum_source_prefix)-h
	@true

stamp-$(enum_source_prefix)-h: $(enum_sources_h) Makefile
	(cd $(srcdir) && \
	  mark="__`echo $(enum_source_prefix) | sed -e 's/-/_/g' | tr a-z A-Z`_H__" && \
	  $(GLIB_MKENUMS) \
	    --fhead "#ifndef $${mark}\n#define $${mark}\n\n#include <glib-object.h>\n\nG_BEGIN_DECLS\n" \
	    --fprod "/* enumerations from \"@filename@\" */\n" \
	    --vhead "GType @enum_name@_get_type (void);\n#define MC_TYPE_@ENUMSHORT@ (@enum_name@_get_type())\n" 	\
	    --ftail "G_END_DECLS\n\n#endif /* $${mark} */" \
	    $(enum_sources_h)) > tmp-$(enum_source_prefix).h && \
	(cmp -s tmp-$(enum_source_prefix).h $(enum_source_prefix).h || \
	  cp tmp-$(enum_source_prefix).h $(enum_source_prefix).h) && \
	rm -f tmp-$(enum_source_prefix).h && \
	echo timestamp > $(@F)

mc-marshalers.h : mc-marshalers.list
	$(GLIB_GENMARSHAL) $(srcdir)/mc-marshalers.list --header --prefix=_mc_marshal > $@

mc-marshalers.c : mc-marshalers.list
	$(GLIB_GENMARSHAL) $(srcdir)/mc-marshalers.list --header --body --prefix=_mc_marshal > $@
