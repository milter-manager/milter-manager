AM_CPPFLAGS =						\
	 -I$(top_builddir)				\
	 -I$(top_srcdir)

AM_CFLAGS = 						\
	-DLOCALEDIR=\""$(localedir)"\"			\
	-DPREFIX=\""$(prefix)"\"			\
	-DMILTER_LOG_DOMAIN=\""milter-manager"\"	\
	$(MILTER_MANAGER_CFLAGS)			\
	$(COVERAGE_CFLAGS)

LIBS =							\
	$(MILTER_MANAGER_LIBS)

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
	milter-manager-module.h				\
	milter-manager-module-impl.h			\
	milter-manager-control-command-decoder.h	\
	milter-manager-control-reply-decoder.h		\
	milter-manager-control-command-encoder.h	\
	milter-manager-control-reply-encoder.h		\
	milter-manager-control-protocol.h		\
	milter-manager-reply-protocol.h			\
	milter-manager-reply-decoder.h			\
	milter-manager-reply-encoder.h			\
	milter-manager-controller-context.h		\
	milter-manager-controller.h			\
	milter-manager-launch-protocol.h		\
	milter-manager-launch-command-encoder.h		\
	milter-manager-launch-command-decoder.h		\
	milter-manager-applicable-condition.h		\
	milter-manager-process-launcher.h		\
	milter-manager.h

enum_source_prefix = milter-manager-enum-types
enum_sources_h =			\
	$(milter_manager_public_headers)

milter_manager_includedir=$(pkgincludedir)/milter/manager
milter_manager_include_HEADERS =	\
	milter-manager-enum-types.h	\
	$(milter_manager_public_headers)

lib_LTLIBRARIES =		\
	libmilter-manager.la

libmilter_manager_la_SOURCES =				\
	milter-manager-enum-types.c			\
	milter-manager.c				\
	milter-manager-main.c				\
	milter-manager-configuration.c			\
	milter-manager-child.c				\
	milter-manager-children.c			\
	milter-manager-module.c				\
	milter-manager-leader.c				\
	milter-manager-egg.c				\
	milter-manager-control-command-decoder.c	\
	milter-manager-control-reply-decoder.c		\
	milter-manager-control-command-encoder.c	\
	milter-manager-control-reply-encoder.c		\
	milter-manager-controller-context.c		\
	milter-manager-controller.c			\
	milter-manager-reply-decoder.c			\
	milter-manager-reply-encoder.c			\
	milter-manager-launch-command-encoder.c		\
	milter-manager-launch-command-decoder.c		\
	milter-manager-applicable-condition.c		\
	milter-manager-process-launcher.c

libmilter_manager_la_LIBADD =					\
	$(top_builddir)/milter/client/libmilter-client.la	\
	$(top_builddir)/milter/server/libmilter-server.la	\
	$(MILTER_MANAGER_LIBS)

include $(top_srcdir)/build/enum-types.mk
