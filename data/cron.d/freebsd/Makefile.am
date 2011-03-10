crondir = $(sysconfdir)/cron.d

cron_script = milter-manager-log

if FREEBSD_PLATFORM
dist_cron_DATA = $(cron_script)
else
EXTRA_DIST = $(cron_script)
endif
