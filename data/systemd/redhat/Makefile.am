service = \
	milter-manager.service

if REDHAT_PLATFORM
initdir = $(sysconfdir)/systemd/redhat
dist_init_SCRIPTS = $(service)
else
EXTRA_DIST = $(service)
endif
