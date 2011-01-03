INCLUDES =					\
	 -I$(top_srcdir)/gio-unix-compatible

AM_CFLAGS =					\
	$(GLIB_CFLAGS)				\
	-DGIO_COMPILATION			\
	-DSIZEOF_CHAR=1

LIBS =						\
	$(GLIB_LIBS)

if !GIO_UNIX_2_26
noinst_LTLIBRARIES =				\
	libgio-compatible.la			\
	libgio-unix-compatible.la

libgio_compatible_la_SOURCES =			\
	gio.h					\
	gcredentials.c				\
	gcredentials.h				\
	gioerror.c				\
	gioerror.h				\
	ginputstream.c				\
	ginputstream.h				\
	goutputstream.c				\
	goutputstream.h				\
	gcancellable.c				\
	gcancellable.h				\
	gseekable.c				\
	gseekable.h				\
	gasyncresult.c				\
	gasyncresult.h				\
	gsimpleasyncresult.c			\
	gsimpleasyncresult.h			\
	gioscheduler.c				\
	gioscheduler.h				\
	giostream.c				\
	giostream.h				\
	gsocket.c				\
	gsocket.h				\
	gsocketconnection.c			\
	gsocketconnection.h			\
	gsocketconnectable.c			\
	gsocketconnectable.h			\
	gsocketcontrolmessage.c			\
	gsocketcontrolmessage.h			\
	gsocketaddress.c			\
	gsocketaddress.h			\
	gsocketinputstream.c			\
	gsocketinputstream.h			\
	gsocketoutputstream.c			\
	gsocketoutputstream.h			\
	ginetaddress.c				\
	ginetaddress.h				\
	gtcpconnection.c			\
	gtcpconnection.h			\
	gunixsocketaddress.c			\
	gunixsocketaddress.h			\
	ginetsocketaddress.c			\
	ginetsocketaddress.h			\
	gsocketaddressenumerator.c		\
	gsocketaddressenumerator.h		\
	gnetworkaddress.c			\
	gnetworkaddress.h			\
	gsrvtarget.c				\
	gsrvtarget.h				\
	ginitable.c				\
	ginitable.h				\
	gioenumtypes.c				\
	gioenumtypes.h				\
	giotypes.h				\
	gioenums.h				\
	gnetworkingprivate.h			\
	glibintl.h

libgio_unix_compatible_la_SOURCES =		\
	gunixconnection.c			\
	gunixconnection.h			\
	gunixfdmessage.c			\
	gunixfdmessage.h			\
	gunixfdlist.c				\
	gunixfdlist.h

libgio_unix_compatible_la_LIBADD =		\
        libgio-compatible.la
endif
