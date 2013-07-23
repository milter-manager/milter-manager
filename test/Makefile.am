SUBDIRS =		\
	lib		\
	fixtures	\
	core		\
	client		\
	libmilter	\
	server		\
	manager		\
	tool

if WITH_CUTTER
TESTS = run-test.sh
TESTS_ENVIRONMENT =				\
	NO_MAKE=yes				\
	CUTTER="$(CUTTER)"			\
	RUBY="$(RUBY)"

noinst_LTLIBRARIES =			\
	suite-milter-manager-test.la
endif

EXTRA_DIST =		\
	run-test.sh

AM_CPPFLAGS =		\
	-I$(srcdir)	\
	-I$(top_srcdir)

AM_CFLAGS = 				\
	$(MILTER_MANAGER_CFLAGS)

AM_LDFLAGS =			\
	-module			\
	-rpath "`pwd`/.libs"	\
	-avoid-version		\
	-no-undefined

LIBS =								\
	$(top_builddir)/milter/manager/libmilter-manager.la

suite_milter_manager_test_la_SOURCES = suite-milter-manager-test.c

echo-abs-top-builddir:
	@echo $(abs_top_builddir)
