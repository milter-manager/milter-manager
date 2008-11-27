if WITH_CUTTER
noinst_LTLIBRARIES =			\
	libmilter-test.la		\
	libmilter-manager-test.la
endif

EXTRA_DISTS =			\
	milter-test-server.rb

INCLUDES =			\
	-I$(top_srcdir)

AM_CFLAGS = 				\
	-DCUT_RELATIVE_PATH="\"lib\""	\
	$(GCUTTER_CFLAGS)

AM_LDFLAGS =			\
	-module			\
	-rpath "`pwd`/.libs"	\
	-avoid-version		\
	-no-undefined

LIBS =			\
	$(GCUTTER_LIBS)

libmilter_test_la_SOURCES =		\
	milter-test-utils.c		\
	milter-test-utils.h		\
	milter-test-client.c		\
	milter-test-client.h		\
	milter-test-server.c		\
	milter-test-server.h		\
	milter-assertions.c		\
	milter-assertions.h

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
