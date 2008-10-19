if WITH_CUTTER
noinst_LTLIBRARIES =			\
	test-configuration.la
endif

INCLUDES =				\
	-I$(srcdir)			\
	-I$(top_srcdir)/test/lib	\
	-I$(top_srcdir)

AM_CFLAGS = 				\
	$(MILTER_MANAGER_CFLAGS)	\
	$(GCUTTER_CFLAGS)

AM_LDFLAGS =			\
	-module			\
	-rpath "`pwd`/.libs"	\
	-avoid-version		\
	-no-undefined

LIBS =								\
	$(top_builddir)/milter/manager/libmilter-manager.la	\
	$(top_builddir)/test/lib/libmilter-manager-test.la	\
	$(GCUTTER_LIBS)

test_configuration_la_SOURCES = test-configuration.c
