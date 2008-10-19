if WITH_CUTTER
noinst_LTLIBRARIES =		\
	libmilter-manager-test.la
endif

INCLUDES =				\
	-I$(top_srcdir)

AM_CFLAGS = $(GCUTTER_CFLAGS)
AM_LDFLAGS =			\
	-avoid-version		\
	-no-undefined

LIBS =							\
	$(GCUTTER_LIBS)

libmilter_manager_test_la_SOURCES =	\
	milter-manager-test-utils.c	\
	milter-manager-test-utils.h
