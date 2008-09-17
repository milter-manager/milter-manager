if WITH_CUTTER
noinst_LTLIBRARIES =		\
	test-parser.la		\
	test-option.la
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

LIBS =							\
	$(top_builddir)/milter-core/libmilter-core.la	\
	$(GCUTTER_LIBS)

test_parser_la_SOURCES = test-parser.c
test_option_la_SOURCES = test-option.c
