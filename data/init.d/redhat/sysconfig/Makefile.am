sysconfigdir = $(sysconfdir)/sysconfig

default_conf = milter-manager

if REDHAT_PLATFORM
dist_sysconfig_DATA = $(default_conf)
else
EXTRA_DIST = $(default_conf)
endif
