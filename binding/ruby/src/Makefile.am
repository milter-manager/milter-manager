INCLUDES = 				\
	 -I$(top_srcdir)

AM_CFLAGS = 				\
	$(MILTER_MANAGER_CFLAGS)

LIBS =								\
	$(MILTER_MANAGER_LIBS)					\
	$(top_builddir)/milter/manager/libmilter-manager.la

AM_LDFLAGS =			\
	-module			\
	-avoid-version		\
	-no-undefined

CLEANFILES = *.gcno *.gcda

binding_ext_LTLIBRARIES =	\
	milter_manager.la

milter_manager_la_SOURCES =				\
	rb-milter-manager.h				\
	rb-milter-manager-private.h			\
	rb-milter-manager.c				\
	rb-milter-manager-configuration.c		\
	rb-milter-manager-child.c			\
	rb-milter-manager-egg.c				\
	rb-milter-manager-children.c			\
	rb-milter-manager-control-command-encoder.c
