AM_CPPFLAGS =					\
	-I$(top_srcdir)				\
	-I$(top_srcdir)/binding/ruby/ext/core

AM_CFLAGS =						\
	-DMILTER_LOG_DOMAIN=\""ruby-milter-server"\"	\
	$(MILTER_TOOLKIT_CFLAGS)			\
	$(MILTER_RUBY_CFLAGS)

LIBS =								\
	$(top_builddir)/milter/core/libmilter-core.la		\
	$(top_builddir)/milter/server/libmilter-server.la	\
	$(MILTER_RUBY_LIBS)

AM_LDFLAGS =			\
	-module			\
	-avoid-version

CLEANFILES = *.gcno *.gcda

binding_ext_LTLIBRARIES =	\
	milter_server.la

binding_ext_HEADERS =				\
	rb-milter-server.h

noinst_HEADERS =				\
	rb-milter-server-private.h

milter_server_la_SOURCES =			\
	rb-milter-server.c			\
	rb-milter-server-context.c
