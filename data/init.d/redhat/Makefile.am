SUBDIRS = sysconfig

init_script = milter-manager

if REDHAT_PLATFORM
initdir = $(sysconfdir)/rc.d/init.d
dist_init_SCRIPTS = $(init_script)
else
EXTRA_DIST = $(init_script)
endif
