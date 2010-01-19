
SHELL = /bin/sh

#### Start of system configuration section. ####

srcdir = /home/kou/work/ruby/ruby-gnome2.clean/glib/src
topdir = /usr/lib/ruby/1.8/x86_64-linux
hdrdir = $(topdir)
VPATH = $(srcdir):$(topdir):$(hdrdir)
exec_prefix = $(prefix)
prefix = $(DESTDIR)/usr
sharedstatedir = $(prefix)/com
mandir = $(prefix)/share/man
psdir = $(docdir)
oldincludedir = $(DESTDIR)/usr/include
localedir = $(datarootdir)/locale
bindir = $(exec_prefix)/bin
libexecdir = $(prefix)/lib/ruby1.8
sitedir = $(DESTDIR)/usr/local/lib/site_ruby
htmldir = $(docdir)
vendorarchdir = $(vendorlibdir)/$(sitearch)
includedir = $(prefix)/include
infodir = $(prefix)/share/info
vendorlibdir = $(vendordir)/$(ruby_version)
sysconfdir = $(DESTDIR)/etc
libdir = $(exec_prefix)/lib
sbindir = $(exec_prefix)/sbin
rubylibdir = $(libdir)/ruby/$(ruby_version)
docdir = $(datarootdir)/doc/$(PACKAGE)
dvidir = $(docdir)
vendordir = $(libdir)/ruby/vendor_ruby
datarootdir = $(prefix)/share
pdfdir = $(docdir)
archdir = $(rubylibdir)/$(arch)
sitearchdir = $(sitelibdir)/$(sitearch)
datadir = $(datarootdir)
localstatedir = $(DESTDIR)/var
sitelibdir = $(sitedir)/$(ruby_version)

CC = cc
LIBRUBY = $(LIBRUBY_SO)
LIBRUBY_A = lib$(RUBY_SO_NAME)-static.a
LIBRUBYARG_SHARED = -l$(RUBY_SO_NAME)
LIBRUBYARG_STATIC = -l$(RUBY_SO_NAME)-static

RUBY_EXTCONF_H = 
CFLAGS   =  -fPIC -fno-strict-aliasing -g -g -O2  -fPIC $(cflags) -Wall -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include   -pthread -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include   
INCFLAGS = -I. -I. -I/usr/lib/ruby/1.8/x86_64-linux -I/home/kou/work/ruby/ruby-gnome2.clean/glib
DEFS     = 
CPPFLAGS = -DHAVE_RB_DEFINE_ALLOC_FUNC -DHAVE_RB_BLOCK_PROC -DHAVE_OBJECT_ALLOCATE -DHAVE_NODE_ATTRASGN -DHAVE_UNISTD_H -DHAVE_G_SPAWN_CLOSE_PID -DHAVE_G_THREAD_INIT -DHAVE_G_MAIN_DEPTH -DHAVE_G_LISTENV -DHAVE_RB_CHECK_ARRAY_TYPE -DHAVE_RB_EXEC_RECURSIVE -DHAVE_RUBY_SET_CURRENT_SOURCE -DHAVE_RB_CURR_THREAD -DRUBY_GLIB2_COMPILATION   
CXXFLAGS = $(CFLAGS) 
ldflags  = -L.  -rdynamic -Wl,-export-dynamic  -pthread
dldflags = 
archflag = 
DLDFLAGS = $(ldflags) $(dldflags) $(archflag)
LDSHARED = $(CC) -shared
AR = ar
EXEEXT = 

RUBY_INSTALL_NAME = ruby1.8
RUBY_SO_NAME = ruby1.8
arch = x86_64-linux
sitearch = x86_64-linux
ruby_version = 1.8
ruby = /usr/bin/ruby1.8
RUBY = $(ruby)
RM = rm -f
MAKEDIRS = mkdir -p
INSTALL = /usr/bin/install -c
INSTALL_PROG = $(INSTALL) -m 0755
INSTALL_DATA = $(INSTALL) -m 644
COPY = cp

#### End of system configuration section. ####

preload = 

libpath = . $(libdir)
LIBPATH =  -L. -L$(libdir)
DEFFILE = 

CLEANFILES = mkmf.log
DISTCLEANFILES = glib-enum-types.h glib-enum-types.c

extout = 
extout_prefix = 
target_prefix = 
LOCAL_LIBS = 
LIBS = $(LIBRUBYARG_SHARED)  -lgobject-2.0 -lglib-2.0   -lgthread-2.0 -lrt -lglib-2.0   -lpthread -lrt -ldl -lcrypt -lm   -lc
SRCS = rbglib.c rbgobj_fundamental.c rbglib_shell.c rbglib_utils.c rbgobj_paramspecs.c rbglib_messages.c rbglib_i18n.c glib-enum-types.c rbglib_fileutils.c rbglib_timer.c rbgobj_value.c rbglib_iochannel.c rbgobj_typeinterface.c rbglib_win32.c rbglib_threads.c rbglib_maincontext.c rbgobj_typeinstance.c rbglib_mainloop.c rbglib_pollfd.c rbgobj_enums.c rbglib_unicode.c rbgobj_strv.c rbgobj_signal.c rbglib_keyfile.c rbgobj_closure.c rbgobj_object.c rbglib_source.c rbgobj_boxed.c rbglib_int64.c rbglib_completion.c rbglib_convert.c rbgobj_param.c rbglib_bookmarkfile.c rbgobj_valuearray.c rbglib_error.c rbgobj_convert.c rbgutil.c rbgobject.c rbglib_spawn.c rbgobj_typemodule.c rbgobj_type.c rbgobj_valuetypes.c rbgutil_callback.c rbgobj_typeplugin.c
OBJS = rbglib.o rbgobj_fundamental.o rbglib_shell.o rbglib_utils.o rbgobj_paramspecs.o rbglib_messages.o rbglib_i18n.o glib-enum-types.o rbglib_fileutils.o rbglib_timer.o rbgobj_value.o rbglib_iochannel.o rbgobj_typeinterface.o rbglib_win32.o rbglib_threads.o rbglib_maincontext.o rbgobj_typeinstance.o rbglib_mainloop.o rbglib_pollfd.o rbgobj_enums.o rbglib_unicode.o rbgobj_strv.o rbgobj_signal.o rbglib_keyfile.o rbgobj_closure.o rbgobj_object.o rbglib_source.o rbgobj_boxed.o rbglib_int64.o rbglib_completion.o rbglib_convert.o rbgobj_param.o rbglib_bookmarkfile.o rbgobj_valuearray.o rbglib_error.o rbgobj_convert.o rbgutil.o rbgobject.o rbglib_spawn.o rbgobj_typemodule.o rbgobj_type.o rbgobj_valuetypes.o rbgutil_callback.o rbgobj_typeplugin.o
TARGET = glib2
DLLIB = $(TARGET).so
EXTSTATIC = 
STATIC_LIB = 

BINDIR        = $(bindir)
RUBYCOMMONDIR = $(sitedir)$(target_prefix)
RUBYLIBDIR    = $(sitelibdir)$(target_prefix)
RUBYARCHDIR   = $(sitearchdir)$(target_prefix)

TARGET_SO     = $(DLLIB)
CLEANLIBS     = $(TARGET).so $(TARGET).il? $(TARGET).tds $(TARGET).map
CLEANOBJS     = *.o *.a *.s[ol] *.pdb *.exp *.bak

all:		$(DLLIB)
static:		$(STATIC_LIB)

clean:
		@-$(RM) $(CLEANLIBS) $(CLEANOBJS) $(CLEANFILES)

distclean:	clean
		@-$(RM) Makefile $(RUBY_EXTCONF_H) conftest.* mkmf.log
		@-$(RM) core ruby$(EXEEXT) *~ $(DISTCLEANFILES)

realclean:	distclean
install: install-so install-rb

install-so: $(RUBYARCHDIR)
install-so: $(RUBYARCHDIR)/$(DLLIB)
$(RUBYARCHDIR)/$(DLLIB): $(DLLIB)
	$(INSTALL_PROG) $(DLLIB) $(RUBYARCHDIR)
install-rb: pre-install-rb install-rb-default
install-rb-default: pre-install-rb-default
pre-install-rb: Makefile
pre-install-rb-default: Makefile
pre-install-rb-default: $(RUBYLIBDIR)
install-rb-default: $(RUBYLIBDIR)/glib2.rb
$(RUBYLIBDIR)/glib2.rb: /home/kou/work/ruby/ruby-gnome2.clean/glib/src/lib/glib2.rb $(RUBYLIBDIR)
	$(INSTALL_DATA) /home/kou/work/ruby/ruby-gnome2.clean/glib/src/lib/glib2.rb $(@D)
install-rb-default: $(RUBYLIBDIR)/pkg-config.rb
$(RUBYLIBDIR)/pkg-config.rb: /home/kou/work/ruby/ruby-gnome2.clean/glib/src/lib/pkg-config.rb $(RUBYLIBDIR)
	$(INSTALL_DATA) /home/kou/work/ruby/ruby-gnome2.clean/glib/src/lib/pkg-config.rb $(@D)
install-rb-default: $(RUBYLIBDIR)/mkmf-gnome2.rb
$(RUBYLIBDIR)/mkmf-gnome2.rb: /home/kou/work/ruby/ruby-gnome2.clean/glib/src/lib/mkmf-gnome2.rb $(RUBYLIBDIR)
	$(INSTALL_DATA) /home/kou/work/ruby/ruby-gnome2.clean/glib/src/lib/mkmf-gnome2.rb $(@D)
install-rb-default: $(RUBYLIBDIR)/glib-mkenums.rb
$(RUBYLIBDIR)/glib-mkenums.rb: /home/kou/work/ruby/ruby-gnome2.clean/glib/src/lib/glib-mkenums.rb $(RUBYLIBDIR)
	$(INSTALL_DATA) /home/kou/work/ruby/ruby-gnome2.clean/glib/src/lib/glib-mkenums.rb $(@D)
$(RUBYARCHDIR):
	$(MAKEDIRS) $@
$(RUBYLIBDIR):
	$(MAKEDIRS) $@

site-install: site-install-so site-install-rb
site-install-so: install-so
site-install-rb: install-rb

.SUFFIXES: .c .m .cc .cxx .cpp .C .o

.cc.o:
	$(CXX) $(INCFLAGS) $(CPPFLAGS) $(CXXFLAGS) -c $<

.cxx.o:
	$(CXX) $(INCFLAGS) $(CPPFLAGS) $(CXXFLAGS) -c $<

.cpp.o:
	$(CXX) $(INCFLAGS) $(CPPFLAGS) $(CXXFLAGS) -c $<

.C.o:
	$(CXX) $(INCFLAGS) $(CPPFLAGS) $(CXXFLAGS) -c $<

.c.o:
	$(CC) $(INCFLAGS) $(CPPFLAGS) $(CFLAGS) -c $<

$(DLLIB): $(OBJS) Makefile
	@-$(RM) $@
	$(LDSHARED) -o $@ $(OBJS) $(LIBPATH) $(DLDFLAGS) $(LOCAL_LIBS) $(LIBS)



###
install-so:
	$(INSTALL_DATA) $(srcdir)/rbglib.h $(RUBYARCHDIR)
	$(INSTALL_DATA) $(srcdir)/rbgutil.h $(RUBYARCHDIR)
	$(INSTALL_DATA) $(srcdir)/rbgobject.h $(RUBYARCHDIR)
	$(INSTALL_DATA) $(srcdir)/rbgcompat.h $(RUBYARCHDIR)
	$(INSTALL_DATA) glib-enum-types.h $(RUBYARCHDIR)

install:
	$(MAKEDIRS) $(libdir)/pkgconfig
	$(INSTALL_DATA) ../ruby-glib2.pc $(libdir)/pkgconfig
