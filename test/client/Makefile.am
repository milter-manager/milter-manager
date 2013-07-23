if WITH_CUTTER
noinst_LTLIBRARIES =			\
	test-client-context-base.la	\
	test-client-context-callback.la	\
	test-client-context-data.la	\
	test-client-context-body.la	\
	test-client-context-reply.la	\
	test-client-context-address.la	\
	test-client-context-modify.la	\
	test-client.la
endif

AM_CPPFLAGS =				\
	-I$(srcdir)			\
	-I$(srcdir)/lib			\
	-I$(top_srcdir)			\
	-I$(top_srcdir)/test/lib	\
	-I$(top_srcdir)/milter-core	\
	-I$(top_srcdir)/milter-client

AM_CFLAGS = $(GCUTTER_CFLAGS)
AM_LDFLAGS =			\
	-module			\
	-rpath "`pwd`/.libs"	\
	-avoid-version		\
	-no-undefined

LIBS =								\
	$(top_builddir)/milter/client/libmilter-client.la	\
	$(top_builddir)/test/lib/libmilter-test.la		\
	$(GCUTTER_LIBS)

test_client_context_base_la_SOURCES     = test-client-context-base.c
test_client_context_callback_la_SOURCES = test-client-context-callback.c
test_client_context_data_la_SOURCES     = test-client-context-data.c
test_client_context_body_la_SOURCES     = test-client-context-body.c
test_client_context_reply_la_SOURCES    = test-client-context-reply.c
test_client_context_address_la_SOURCES  = test-client-context-address.c
test_client_context_modify_la_SOURCES   = test-client-context-modify.c
test_client_la_SOURCES                  = test-client.c
