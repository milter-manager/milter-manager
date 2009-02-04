INCLUDES =						\
	 -I$(top_srcdir)				\
	 -I$(top_srcdir)/binding/ruby/src/toolkit

AM_CFLAGS = 				\
	$(MILTER_MANAGER_CFLAGS)	\
	$(RUBY_CFLAGS)

LIBS =			\
	$(RUBY_LIBS)

AM_LDFLAGS =			\
	-module			\
	-avoid-version		\
	-no-undefined

CLEANFILES = *.gcno *.gcda

binding_ext_LTLIBRARIES =	\
	milter_manager.la

noinst_HEADERS =				\
	rb-milter-manager.h			\
	rb-milter-manager-private.h

milter_manager_la_SOURCES =				\
	rb-milter-manager.c				\
	rb-milter-manager-gstring.c			\
	rb-milter-manager-configuration.c		\
	rb-milter-manager-child.c			\
	rb-milter-manager-egg.c				\
	rb-milter-manager-children.c			\
	rb-milter-manager-control-command-encoder.c	\
	rb-milter-manager-control-reply-encoder.c	\
	rb-milter-manager-control-decoder.c		\
	rb-milter-manager-applicable-condition.c

milter_manager_la_LIBADD =					\
	$(top_builddir)/milter/manager/libmilter-manager.la
