if WITH_CUTTER
noinst_LTLIBRARIES =			\
	libmilter-test.la		\
	libmilter-manager-test.la
endif

dist_noinst_SCRIPTS =		\
	milter-test-server.rb	\
	milter-test-client.rb

MAINTAINERCLEANFILES =
BUILT_SOURCES =

enum_source_prefix = milter-test-enum-types
enum_sources_h =			\
	$(libmilter_test_headers)

AM_CPPFLAGS =			\
	-I$(top_srcdir)

AM_CFLAGS = 				\
	-DCUT_RELATIVE_PATH="\"lib\""	\
	$(GCUTTER_CFLAGS)

AM_LDFLAGS =			\
	-rpath "`pwd`/.libs"	\
	-avoid-version		\
	-no-undefined

LIBS =			\
	$(GCUTTER_LIBS)

libmilter_test_headers =	\
	milter-test-utils.h	\
	milter-test-client.h	\
	milter-test-server.h

libmilter_test_la_SOURCES =		\
	$(libmilter_test_headers)	\
	milter-test-compatible.h	\
	milter-test-utils.c		\
	milter-test-client.c		\
	milter-test-server.c		\
	milter-test-glib-compatible.c	\
	milter-test-glib-compatible.h	\
	milter-test-enum-types.c	\
	milter-test-enum-types.h	\
	milter-assertions.c		\
	milter-assertions.h		\
	gcut-milter-event-loop.c	\
	gcut-milter-event-loop.h

libmilter_manager_test_la_SOURCES =	\
	milter-manager-test-utils.c	\
	milter-manager-test-utils.h	\
	milter-manager-test-client.c	\
	milter-manager-test-client.h	\
	milter-manager-test-server.c	\
	milter-manager-test-server.h	\
	milter-manager-test-scenario.c	\
	milter-manager-test-scenario.h

libmilter_manager_test_la_CFLAGS =	\
	-DCUT_RELATIVE_PATH="\"lib\""	\
	$(GCUTTER_CFLAGS)		\
	$(MILTER_MANAGER_CFLAGS)

libmilter_manager_test_la_LIBS =			\
	$(top_builddir)/milter/core/libmilter-core.la

include $(top_srcdir)/build/enum-types.mk
