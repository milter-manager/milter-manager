INCLUDES = 					\
	 -I$(top_builddir)			\
	 -I$(top_srcdir)			\
	 -I$(top_srcdir)/binding/ruby/src

AM_CFLAGS = 						\
	-DLOCALEDIR=\""$(localedir)"\"			\
	-DMILTER_LOG_DOMAIN=\""milter-manager"\"	\
	$(MILTER_MANAGER_CFLAGS)			\
	$(GLIB_CFLAGS)					\
	$(COVERAGE_CFLAGS)

LIBS =					\
	$(MILTER_MANAGER_LIBS)		\
	$(GLIB_LIBS)

CLEANFILES = *.gcno *.gcda

EXTRA_DIST =
MAINTAINERCLEANFILES =
BUILT_SOURCES =

milter_manager_public_headers =				\
	milter-manager-leader.h				\
	milter-manager-configuration.h			\
	milter-manager-child.h				\
	milter-manager-children.h			\
	milter-manager-objects.h			\
	milter-manager-egg.h				\
	milter-manager-headers.h			\
	milter-manager-module.h				\
	milter-manager-module-impl.h			\
	milter-manager-control-command-decoder.h	\
	milter-manager-control-command-encoder.h	\
	milter-manager-control-protocol.h		\
	milter-manager-controller.h

enum_source_prefix = milter-manager-enum-types
enum_sources_h =			\
	$(milter_manager_public_headers)

marshal_source_prefix = milter-manager-marshalers
marshal_prefix = _milter_manager_marshal

milter_manager_includedir=$(pkgincludedir)/milter-manager
milter_manager_include_HEADERS =	\
	milter-manager-enum-types.h	\
	$(milter_manager_public_headers)

lib_LTLIBRARIES =		\
	libmilter-manager.la

libmilter_manager_la_SOURCES =				\
	milter-manager-enum-types.c			\
	milter-manager-marshalers.c			\
	milter-manager-marshalers.h			\
	milter-manager-ruby-internal.h			\
	milter-manager.c				\
	milter-manager-configuration.c			\
	milter-manager-child.c				\
	milter-manager-children.c			\
	milter-manager-module.c				\
	milter-manager-leader.c				\
	milter-manager-egg.c				\
	milter-manager-headers.c			\
	milter-manager-control-command-decoder.c	\
	milter-manager-control-command-encoder.c	\
	milter-manager-controller.c

include $(top_srcdir)/build/enum-types.mk
include $(top_srcdir)/build/marshal.mk
