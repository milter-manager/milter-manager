if WITH_CUTTER
noinst_LTLIBRARIES =			\
	libmilter-manager-test.la
endif

EXTRA_DISTS =			\
	milter-test-server.rb

INCLUDES =			\
	-I$(top_srcdir)

AM_CFLAGS = 			\
	$(GCUTTER_CFLAGS)	\
	$(MILTER_MANAGER_CFLAGS)

AM_LDFLAGS =			\
	-module			\
	-rpath "`pwd`/.libs"	\
	-avoid-version		\
	-no-undefined

LIBS =						\
	$(GCUTTER_LIBS)

libmilter_manager_test_la_SOURCES =	\
	milter-manager-test-utils.c	\
	milter-manager-test-utils.h	\
	milter-manager-test-client.c	\
	milter-manager-test-client.h	\
	milter-manager-test-server.c	\
	milter-manager-test-server.h
