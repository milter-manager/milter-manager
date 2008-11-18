INCLUDES = 					\
	 -I$(top_builddir)			\
	 -I$(top_srcdir)

AM_CFLAGS = 							\
	-DLOCALEDIR=\""$(localedir)"\"				\
	-DMILTER_LOG_DOMAIN=\""milter-manager-test-milter"\"	\
	$(MILTER_MANAGER_CFLAGS)

CLEANFILES = *.gcno *.gcda

bin_PROGRAMS =		\
	milter-manager-test-milter

milter_manager_test_milter_SOURCES =		\
	milter-manager-test-milter.c

milter_manager_test_milter_LDADD =			\
	$(top_builddir)/milter/manager/libmilter-manager.la
