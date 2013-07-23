if WITH_CUTTER
noinst_LTLIBRARIES =			\
	test-smfi-context.la		\
	test-libmilter-compatible.la
endif

AM_CPPFLAGS =				\
	-I$(srcdir)			\
	-I$(srcdir)/lib			\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/test/lib

AM_CFLAGS = $(GCUTTER_CFLAGS)
AM_LDFLAGS =			\
	-module			\
	-rpath "`pwd`/.libs"	\
	-avoid-version		\
	-no-undefined

LIBS =							\
	$(top_builddir)/libmilter/libmilter.la		\
	$(top_builddir)/test/lib/libmilter-test.la	\
	$(GCUTTER_LIBS)

test_smfi_context_la_SOURCES		= test-smfi-context.c
test_libmilter_compatible_la_SOURCES	= test-libmilter-compatible.c
