SUBDIRS = sysconfig

initdir = $(sysconfdir)/rc.d/init.d
init_script = milter-manager

if REDHAT_PLATFORM
dist_init_SCRIPTS = $(init_script)
else
EXTRA_DIST = $(init_script)
endif
