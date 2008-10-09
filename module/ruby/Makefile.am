INCLUDES = 					\
	 -I$(top_builddir)			\
	 -I$(top_srcdir)			\
	 -I$(top_srcdir)/binding/ruby/src

AM_CFLAGS = 				\
	-DLOCALEDIR=\""$(localedir)"\"	\
	$(MILTER_MANAGER_CFLAGS)

LIBS =					\
	$(MILTER_MANAGER_LIBS)

AM_LDFLAGS =			\
	-module			\
	-rpath "`pwd`/.libs"	\
	-avoid-version		\
	-no-undefined

CLEANFILES = *.gcno *.gcda

moduledir = $(pkglibdir)/module
module_LTLIBRARIES =		\
	milter-manager-ruby-controller.la

milter_manager_ruby_controller_la_SOURCES =		\
	milter-manager-ruby-controller.c
