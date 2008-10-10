INCLUDES = 					\
	 -I$(top_builddir)			\
	 -I$(top_srcdir)			\
	 -I$(top_srcdir)/binding/ruby/src

AM_CFLAGS = 								\
	-DMILTER_LOG_DOMAIN=\""milter-manager-ruby-controller"\"	\
	$(MILTER_MANAGER_CFLAGS)

LIBS =					\
	$(MILTER_MANAGER_LIBS)

AM_LDFLAGS =				\
	-module				\
	-avoid-version			\
	-no-undefined

CLEANFILES = *.gcno *.gcda

module_controller_LTLIBRARIES =		\
	milter-manager-ruby-controller.la

milter_manager_ruby_controller_la_SOURCES =	\
	milter-manager-ruby-controller.c
