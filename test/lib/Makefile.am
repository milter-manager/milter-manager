if WITH_CUTTER
noinst_LTLIBRARIES =		\
	libmilter-test.la
endif

INCLUDES =				\
	-I$(top_srcdir)

AM_CFLAGS = $(GCUTTER_CFLAGS)
AM_LDFLAGS =			\
	-avoid-version		\
	-no-undefined

LIBS =							\
	$(GCUTTER_LIBS)

libmilter_test_la_SOURCES =	\
	milter-test-utils.c	\
	milter-test-utils.h
