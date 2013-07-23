SUBDIRS = fixtures

TESTS = run-test.rb

EXTRA_DIST =					\
	run-test.rb				\
	tool-test-utils.rb			\
	test-log-analyzer.rb			\
	test-report-graph-generator.rb

if WITH_CUTTER
noinst_LTLIBRARIES =				\
	test-server.la
endif

AM_CPPFLAGS =				\
	-I$(srcdir)			\
	-I$(srcdir)/lib			\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/milter-core	\
	-I$(top_srcdir)/milter-server	\
	-I$(top_srcdir)/test/lib

AM_CFLAGS = $(GCUTTER_CFLAGS)
AM_LDFLAGS =			\
	-module			\
	-rpath "`pwd`/.libs"	\
	-avoid-version		\
	-no-undefined

LIBS =								\
	$(top_builddir)/milter/server/libmilter-server.la	\
	$(top_builddir)/test/lib/libmilter-test.la		\
	$(GCUTTER_LIBS)

test_server_la_SOURCES = test-server.c
