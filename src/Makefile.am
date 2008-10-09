INCLUDES = 					\
	 -I$(top_builddir)			\
	 -I$(top_srcdir)

AM_CFLAGS = 				\
	-DLOCALEDIR=\""$(localedir)"\"	\
	$(MILTER_MANAGER_CFLAGS)

CLEANFILES = *.gcno *.gcda

bin_PROGRAMS =		\
	milter-manager

milter_manager_SOURCES =		\
	milter-manager.c

milter_manager_LDADD =			\
	$(top_builddir)/milter/manager/libmilter-manager.la
