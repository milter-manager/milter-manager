if WITH_CUTTER
noinst_LTLIBRARIES =			\
	test-client-context-callback.la	\
	test-client-context-data.la
endif

INCLUDES =				\
	-I$(srcdir)			\
	-I$(srcdir)/lib			\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/milter-core	\
	-I$(top_srcdir)/milter-client

AM_CFLAGS = $(GCUTTER_CFLAGS)
AM_LDFLAGS =			\
	-module			\
	-rpath "`pwd`/.libs"	\
	-avoid-version		\
	-no-undefined

LIBS =								\
	$(top_builddir)/milter-client/libmilter-client.la	\
	$(GCUTTER_LIBS)

test_client_context_callback_la_SOURCES = test-client-context-callback.c
test_client_context_data_la_SOURCES = test-client-context-data.c
