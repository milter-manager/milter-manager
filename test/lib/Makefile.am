if WITH_CUTTER
noinst_LTLIBRARIES =		\
	libmilter-test.la
endif

INCLUDES =				\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/milter-core

AM_CFLAGS = $(GCUTTER_CFLAGS)
AM_LDFLAGS =			\
	-avoid-version		\
	-no-undefined

LIBS =							\
	$(top_builddir)/milter-core/libmilter-core.la	\
	$(GCUTTER_LIBS)

libmilter_test_la_SOURCES =	\
	milter-test-utils.c	\
	milter-test-utils.h
