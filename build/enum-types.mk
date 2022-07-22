# -*- Mode: Makefile; tab-width: 8; indent-tabs-mode: t; -*-

BUILT_SOURCES +=				\
	$(enum_source_prefix).c			\
	$(enum_source_prefix).h			\
	stamp-$(enum_source_prefix)-c		\
	stamp-$(enum_source_prefix)-h

CLEANFILES +=					\
	$(enum_source_prefix).c			\
	$(enum_source_prefix).h			\
	stamp-$(enum_source_prefix)-c		\
	stamp-$(enum_source_prefix)-h

enum_types_mk = $(top_srcdir)/build/enum-types.mk
enum_types_template_c = $(top_srcdir)/build/enum-types-template.c
enum_types_template_h = $(top_srcdir)/build/enum-types-template.h
abs_enum_types_template_h = $(abs_top_srcdir)/build/enum-types-template.h

$(enum_source_prefix).c: stamp-$(enum_source_prefix)-c $(enum_source_prefix).h
	@true

stamp-$(enum_source_prefix)-c: $(enum_sources_h) $(enum_types_mk) $(enum_types_template_c)
	$(AM_V_GEN) \
	sed -e 's/@enum_types_header@/$(enum_source_prefix).h/g' \
	  $(enum_types_template_c) > enum-types-template.c && \
	abs_build_enum_types_template_c=$(abs_builddir)/enum-types-template.c && \
	(cd $(srcdir) && \
	  $(GLIB_MKENUMS) \
	    --template $${abs_build_enum_types_template_c} \
	    $(enum_sources_h)) > tmp-$(enum_source_prefix).c && \
	(cmp -s tmp-$(enum_source_prefix).c $(enum_source_prefix).c || \
	  cp tmp-$(enum_source_prefix).c $(enum_source_prefix).c ) && \
	rm -f tmp-$(enum_source_prefix).c && \
	echo timestamp > $(@F)

$(enum_source_prefix).h: stamp-$(enum_source_prefix)-h
	@true

stamp-$(enum_source_prefix)-h: $(enum_sources_h) $(enum_types_mk) $(enum_types_template_h)
	$(AM_V_GEN) \
	(cd $(srcdir) && \
	  $(GLIB_MKENUMS) \
	    --template $(abs_enum_types_template_h) \
	    $(enum_sources_h)) > tmp-$(enum_source_prefix).h && \
	(cmp -s tmp-$(enum_source_prefix).h $(enum_source_prefix).h || \
	  cp tmp-$(enum_source_prefix).h $(enum_source_prefix).h) && \
	rm -f tmp-$(enum_source_prefix).h && \
	echo timestamp > $(@F)
