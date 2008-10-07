INCLUDES = 				\
	 -I$(top_builddir)		\
	 -I$(top_srcdir)

AM_CFLAGS = 				\
	-DLOCALEDIR=\""$(localedir)"\"	\
	$(MILTER_MANAGER_CFLAGS)

EXTRA_DIST =
MAINTAINERCLEANFILES =
BUILT_SOURCES =

milter_manager_public_headers =		\
	milter-manager-context.h	\
	milter-manager-configuration.h	\
	milter-manager-child-milter.h	\
	milter-manager-ruby.h

enum_source_prefix = milter-manager-enum-types
enum_sources_h =			\
	$(milter_manager_public_headers)

marshal_source_prefix = milter-manager-marshalers
marshal_prefix = _milter_manager_marshal

milter_manager_includedir=$(pkgincludedir)/milter-manager
milter_manager_include_HEADERS =	\
	milter-manager-enum-types.h	\
	$(milter_manager_public_headers)

bin_PROGRAMS =		\
	milter-manager

milter_manager_SOURCES =		\
	milter-manager-enum-types.c	\
	milter-manager-marshalers.c	\
	milter-manager-marshalers.h	\
	milter-manager-ruby-internal.h	\
	milter-manager.c		\
	milter-manager-context.c	\
	milter-manager-main.c		\
	milter-manager-configuration.c	\
	milter-manager-child-milter.c	\
	milter-manager-ruby.c

milter_manager_LDADD =			\
	$(MILTER_MANAGER_LIBS)

include $(top_srcdir)/build/enum-types.mk
include $(top_srcdir)/build/marshal.mk
