if WITH_CUTTER
noinst_LTLIBRARIES =			\
	test-mc-parser.la
endif

INCLUDES =			\
	-I$(srcdir)		\
	-I$(srcdir)/lib		\
	-I$(top_srcdir)

AM_CFLAGS = $(GCUTTER_CFLAGS)
AM_LDFLAGS =			\
	-module			\
	-rpath "`pwd`/.libs"	\
	-avoid-version		\
	-no-undefined

LIBS =								\
	$(top_builddir)/milter-client/libmilter-client.la	\
	$(GCUTTER_LIBS)

test_mc_parser_la_SOURCES = test-mc-parser.c
