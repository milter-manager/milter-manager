AM_CPPFLAGS =					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/binding/ruby/ext/core

AM_CFLAGS =						\
	-DMILTER_LOG_DOMAIN=\""ruby-milter-client"\"	\
	$(MILTER_TOOLKIT_CFLAGS)			\
	$(MILTER_RUBY_CFLAGS)

LIBS =								\
	$(top_builddir)/milter/core/libmilter-core.la		\
	$(top_builddir)/milter/client/libmilter-client.la	\
	$(MILTER_RUBY_LIBS)

AM_LDFLAGS =			\
	-module			\
	-avoid-version

CLEANFILES = *.gcno *.gcda

binding_ext_LTLIBRARIES =	\
	milter_client.la

binding_ext_HEADERS =				\
	rb-milter-client.h

noinst_HEADERS =				\
	rb-milter-client-private.h

milter_client_la_SOURCES =			\
	rb-milter-client.c			\
	rb-milter-client-context.c
