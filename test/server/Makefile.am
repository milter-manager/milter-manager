if WITH_CUTTER
noinst_LTLIBRARIES =				\
	test-server-context.la			\
	test-server-context-command.la		\
	test-server-context-signals.la		\
	test-server-context-stop-signals.la	\
	test-server-context-step.la
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

test_server_context_la_SOURCES			= test-server-context.c
test_server_context_command_la_SOURCES		= test-server-context-command.c
test_server_context_signals_la_SOURCES		= test-server-context-signals.c
test_server_context_stop_signals_la_SOURCES	= \
	test-server-context-stop-signals.c
test_server_context_step_la_SOURCES		= test-server-context-step.c
