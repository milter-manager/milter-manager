if WITH_CUTTER
noinst_LTLIBRARIES =			\
	test-smfi-context.la
endif

INCLUDES =				\
	-I$(srcdir)			\
	-I$(srcdir)/lib			\
	-I$(top_srcdir)

AM_CFLAGS = $(GCUTTER_CFLAGS)
AM_LDFLAGS =			\
	-module			\
	-rpath "`pwd`/.libs"	\
	-avoid-version		\
	-no-undefined

LIBS =						\
	$(top_builddir)/libmilter/libmilter.la	\
	$(GCUTTER_LIBS)

test_smfi_context_la_SOURCES = test-smfi-context.c
