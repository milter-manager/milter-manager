rcdir = $(sysconfdir)/rc.d

rc_script = milter-manager

if FREEBSD_PLATFORM
dist_rc_SCRIPTS = $(rc_script)
else
EXTRA_DIST = $(rc_script)
endif
