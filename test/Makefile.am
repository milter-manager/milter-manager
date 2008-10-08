SUBDIRS =		\
	lib

if WITH_CUTTER
TESTS = run-test.sh
TESTS_ENVIRONMENT = NO_MAKE=yes CUTTER="$(CUTTER)"

noinst_LTLIBRARIES =			\
	suite-milter-manager-test.la	\
	test-configuration.la
endif

EXTRA_DIST =		\
	run-test.sh

INCLUDES =		\
	-I$(srcdir)	\
	-I$(srcdir)/lib	\
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
	$(GCUTTER_LIBS)

suite_milter_manager_test_la_SOURCES = suite-milter-manager-test.c
test_configuration_la_SOURCES = test-configuration.c

echo-cutter:
	@echo $(CUTTER)
