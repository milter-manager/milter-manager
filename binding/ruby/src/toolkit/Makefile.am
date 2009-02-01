INCLUDES = 				\
	-I$(top_srcdir)

AM_CFLAGS = 				\
	$(MILTER_RUBY_CFLAGS)

LIBS =								\
	$(top_builddir)/milter/core/libmilter-core.la		\
	$(top_builddir)/milter/client/libmilter-client.la	\
	$(top_builddir)/milter/server/libmilter-server.la	\
	$(MILTER_RUBY_LIBS)

AM_LDFLAGS =			\
	-module			\
	-avoid-version		\
	-no-undefined

CLEANFILES = *.gcno *.gcda

binding_ext_LTLIBRARIES =	\
	milter_toolkit.la

noinst_HEADERS =				\
	rb-milter-core-private.h		\
	rb-milter-client-private.h		\
	rb-milter-server-private.h		\
	rb-milter-private.h			\
	rb-milter-compat.h			\
	rb-milter.h

milter_toolkit_la_SOURCES =			\
	rb-milter-core.c			\
	rb-milter-core-private.c		\
	rb-milter-decoder.c			\
	rb-milter-encoder.c			\
	rb-milter-command-encoder.c		\
	rb-milter-reply-encoder.c		\
	rb-milter-connection.c			\
	rb-milter-protocol.c			\
	rb-milter-socket-address.c		\
	rb-milter-macros-requests.c		\
	rb-milter-option.c			\
	rb-milter-agent.c			\
	rb-milter-protocol-agent.c		\
	rb-milter-utils.c			\
	rb-milter-client.c			\
	rb-milter-client-context.c		\
	rb-milter-server-context.c		\
	rb-milter-server.c			\
	rb-milter-compat.c			\
	rb-milter.c
