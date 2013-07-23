AM_CPPFLAGS = 				\
	-I$(top_srcdir)

AM_CFLAGS =						\
	-DMILTER_LOG_DOMAIN=\""ruby-milter-core"\"	\
	$(MILTER_TOOLKIT_CFLAGS)			\
	$(MILTER_RUBY_CFLAGS)

LIBS =								\
	$(top_builddir)/milter/core/libmilter-core.la		\
	$(MILTER_RUBY_LIBS)

AM_LDFLAGS =			\
	-module			\
	-avoid-version

CLEANFILES = *.gcno *.gcda

binding_ext_LTLIBRARIES =	\
	milter_core.la

binding_ext_HEADERS =				\
	rb-milter-core.h

noinst_HEADERS =				\
	rb-milter-core-private.h		\
	rb-milter-compat.h

milter_core_la_SOURCES =			\
	rb-milter-core.c			\
	rb-milter-core-private.c		\
	rb-milter-logger.c			\
	rb-milter-decoder.c			\
	rb-milter-encoder.c			\
	rb-milter-command-encoder.c		\
	rb-milter-reply-encoder.c		\
	rb-milter-connection.c			\
	rb-milter-protocol.c			\
	rb-milter-socket-address.c		\
	rb-milter-macros-requests.c		\
	rb-milter-option.c			\
	rb-milter-error-emittable.c		\
	rb-milter-agent.c			\
	rb-milter-protocol-agent.c		\
	rb-milter-utils.c			\
	rb-milter-compat.c			\
	rb-milter-memory-profile.c		\
	rb-milter-event-loop.c
