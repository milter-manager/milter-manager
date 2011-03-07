SUBDIRS = default

initdir = $(sysconfdir)/init.d
init_script = milter-manager

if DEBIAN_PLATFORM
dist_init_SCRIPTS = $(init_script)
else
EXTRA_DIST = $(init_script)
endif
