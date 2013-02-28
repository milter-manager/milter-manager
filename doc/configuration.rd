# -*- rd -*-

= Configuration --- How to write milter-manager.conf

== About this document

This document describes how to write milter-manager.conf
that is milter-manager's configuration file.

== Place

It assumes that milter-manager is installed under
/usr/local/. If you specify --prefix=/usr/local option to
configure script or omit the option, milter-manager is
installed under /usr/local/.

In this case, milter-manager's configuration file is placed
at /usr/local/etc/milter-manager/milter-manager.conf.
If you have installed successfully, the file should exist.

== Summary

The beginning milter-manager.conf is the following:

  # -*- ruby -*-

  load("applicable-conditions/*.conf")
  load_default
  load_if_exist("milter-manager.local.conf")

Normally, the part should not be changed. If you need to
change your configuration, you should write your
configurations to "milter-manager.local.conf" file in the
same directory of "milter-manager.conf".

Configuration items are categorized as the followings:

  * ((<Package|.#package>))
  * ((<Security|.#security>))
  * ((<milter-manager|.#milter-manager>))
  * ((<Controller|.#controller>))
  * ((<Built-in applicable conditions|.#built-in-applicable-conditions>))
  * ((<Applicable condition|.#applicable-condition>))
  * ((<Child milter|.#child-milter>))
  * ((<Database|.#database>))

There is a convenient feature of milter-manager. It's
introduced before each items are described the below.

If you run milter-manager with --show-config option, the
current configuration is shown.

  % /usr/local/sbin/milter-manager --show-config
  package.platform = "debian"
  package.options = nil

  security.privilege_mode = false
  security.effective_user = nil
  security.effective_group = nil

  log.level = "default"
  log.use_syslog = true
  log.syslog_facility = "mail"

  manager.connection_spec = nil
  manager.unix_socket_mode = 0660
  manager.unix_socket_group = nil
  manager.remove_unix_socket_on_create = true
  manager.remove_unix_socket_on_close = true
  manager.daemon = false
  manager.pid_file = nil
  manager.maintenance_interval = 10
  manager.suspend_time_on_unacceptable = 5
  manager.max_connections = 0
  manager.custom_configuration_directory = nil
  manager.fallback_status = "accept"
  manager.fallback_status_at_disconnect = "temporary-failure"
  manager.event_loop_backend = "glib"
  manager.n_workers = 0
  manager.packet_buffer_size = 0
  manager.connection_check_interval = 0
  manager.chunk_size = 65535
  manager.max_pending_finished_sessions = 0

  controller.connection_spec = nil
  controller.unix_socket_mode = 0660
  controller.remove_unix_socket_on_create = true
  controller.remove_unix_socket_on_close = true

  define_applicable_condition("S25R") do |condition|
    condition.description = "Selective SMTP Rejection"
  end

  define_applicable_condition("Remote Network") do |condition|
    condition.description = "Check only remote network"
  end

You can confirm the current configuration.

The output format is the same as milter-manager.conf's
syntax. You can refer the output for writing
milter-manager.conf.

Here are descriptions of configuration items.

== [package] Package

: package.platform

   ((*Normally, this item doesn't need to be changed.*))

   milter auto-detect method is different on each
   platform. The auto-detect method assumes that milters are
   installed with package system on its platform. If
   real platform and platform detected by milter-manager are
   different, the auto-detect method doesn't work well.

   Your platform is detected on building milter-manager. You
   can specify correct platform if the detected platform is
   wrong. You will use this item only when the detected
   platform is wrong but you can't build again.

   Here are supported platforms:

     * debian: for Debian series Linux like Debian
       GNU/Linux and Ubuntu Linux.
     * redhat: for RedHat series Linux like CentOS.
     * freebsd: for FreeBSD.
     * pkgsrc: for *BSD that use pkgsrc like NetBSD and
       DragonFly BSD.

   Platform name should be surrounded with '"' (double quote)
   like "debian".

   NOTE: This item should be change ((*before*)) load_default.

   Example:
     package.platform = "pkgsrc"

   Default:
     package.platform = "debian" # depend on your environment.

: package.options

   ((*Normally, this item doesn't need to be changed.*))

   This item is determined on building like package.platform.

   You can pass additional information to milter auto-detect
   method. The format of this item is
   "NAME1=VALUE1,NAME2=VALUE2,...". You can pass zero or
   more information.

   Currently, additional information is only used on
   "pkgsrc" platform. "pkgsrc" platform only uses
   "prefix=/PATH/TO/DIRECTORY/HAS/rc.d". For example, you
   need to specify "prefix=/etc" if you install start-script
   under /etc/rc.d/ directory.

   NOTE: This item should be change ((*before*)) load_default.

   Example:
     package.options = "prefix=/etc,name=value"

   Default:
     package.options = nil # depend on your environment.

== [security] Security

: security.privilege_mode

   Specifies whether run as privilege mode or not. If you
   want to use child milter auto run feature, you need to
   enable the mode.

   It's set true for enabling the mode, false otherwise.

   Example:
     security.privilege_mode = true

   Default:
     security.privilege_mode = false

: security.effective_user

   Specifies effective user of milter-manager process. To switch
   effective user, you need to run milter-manager command as
   root user.

   Effective user is specified like "nobody". User name should
   be surrounded with '"' (double quote). If you don't want to
   specify user, use nil.

   Example:
     security.effective_user = "nobody"

   Default:
     security.effective_user = nil

: security.effective_group

   Specifies effective group of milter-manager process. To switch
   effective group, you need to run milter-manager command as
   root group.

   Effective group is specified like "nogroup". Group name should
   be surrounded with '"' (double quote). If you don't want to
   specify group, use nil.

   Example:
     security.effective_group = "nobody"

   Default:
     security.effective_group = nil

== [log] Log

Since 1.6.6.

: log.level

   Specifies log level. All log levels are independent. You
   can specify log level by combining log levels what you
   need like "info, debug and error levels are needed".

   Here are available log levels:

     : default
        Logs critical, error, warnings, message and
        statistics level messages. It's the default.
     : all
        Logs all messages.
     : critical
        Logs critical messages.
     : error
        Logs error messages.
     : warning
        Logs warnings messages.
     : message
        Logs important messages.
     : info
        Logs normal messages.
     : debug
        Logs debug messages.
     : statistics
        Logs statistics messages.
     : profile
        Logs profile messages.

   Log level should be surrounded with '"' (double quote)
   like "all". To specify some log levels, you need to
   separate each log level by "|" like
   "critical|error|warning".

   Example:
     log.level = "all"        # Logs all messages

   Default:
     log.level = "default"

: log.use_syslog

   Specifies whether syslog is also used.

   It's set true for using syslog, false otherwise.

   Example:
     log.use_syslog = false   # Syslog isn't used.

   Default:
     log.use_syslog = true

: log.syslog_facility

   Specifies syslog facility.

   Here are available facilities and corresponded syslog
   constants:

     : authpriv
        LOG_AUTHPRIV
     : cron
        LOG_CRON
     : daemon
        LOG_DAEMON
     : kern
        LOG_KERN
     : local0
        LOG_LOCAL0
     : local1
        LOG_LOCAL1
     : local2
        LOG_LOCAL2
     : local3
        LOG_LOCAL3
     : local4
        LOG_LOCAL4
     : local5
        LOG_LOCAL5
     : local6
        LOG_LOCAL6
     : local7
        LOG_LOCAL7
     : lpr
        LOG_LPR
     : mail
        LOG_MAIL
     : news
        LOG_NEWS
     : user
        LOG_USER
     : uucp
        LOG_UUCP

   Facility should be surrounded with '"' (double quote)
   like "mail".

   Example:
     log.syslog_facility = "local4"   # LOG_LOCAL4 is used.

   Default:
     log.syslog_facility = "mail"

== [milter-manager] milter-manager

: manager.connection_spec

   Specifies a socket that milter-manager accepts connection
   from MTA.

   Socket is specified like "inet:10025". Socket should be
   surrounded with '"' (double quote). Available socket
   formats are the following:

     * UNIX domain socket: unix:PATH
       * Example: unix:/var/run/milter/milter-manager.sock

     * IPv4 socket: inet:PORT
       * Example: inet:10025

     * IPv4 socket: inet:PORT@HOST
       * Example: inet:10025@localhost

     * IPv4 socket: inet:PORT@[ADDRESS]
       * Example: inet:10025@[127.0.0.1]

     * IPv6 socket: inet6:PORT
       * Example: inet6:10025

     * IPv6 socket: inet6:PORT@HOST
       * Example: inet6:10025@localhost

     * IPv6 socket: inet6:PORT@[ADDRESS]
       * Example: inet6:10025@[::1]

   If security.effective_user and/or
   security.effective_group are specified, UNIX domain
   socket is made with the authority.

   If HOST of IPv4 socket and IPv6 socket is omitted,
   milter-manager accepts connection from all network
   interface. If HOST is specified, milter-manager accepts
   connection from the address.

   Example:
     manager.connection_spec = "unix:/var/run/milter/milter-manager.sock"

   Default:
     manager.connection_spec = "inet:10025@[127.0.0.1]"

: manager.unix_socket_mode

   Specifies permission of UNIX domain socket that
   milter-manager uses for accepting
   connection. This is used only when UNIX domain socket is
   specified for manager.connection_spec.

   Don't forget to prepend '0' to specify permission as
   octal notation.

   Example:
    manager.unix_socket_mode = 0600

   Default:
    manager.unix_socket_mode = 0660

: manager.unix_socket_group

   Specifies group of UNIX domain socket that
   milter-manager uses for accepting
   connection. This is used only when UNIX domain socket is
   specified for manager.connection_spec.

   Socket's group is changed by chown(2) after creating a
   socket with
   security.effective_user/security.effective_group
   authority. Specified group should be one of the
   security.effective_user's supplementary groups.

   Group is specified like "nogroup". Group name should
   be surrounded with '"' (double quote). If you don't want to
   specify group, use nil.

   Example:
     manager.unix_socket_group = "nobody"

   Default:
     manager.unix_socket_group = nil

: manager.remove_unix_socket_on_create

   Specifies whether remove existing UNIX domain socket
   before creating new UNIX domain socket or not. This is
   used only when UNIX domain socket is specified for
   manager.connection_spec.

   True if remove, false otherwise.

   Example:
     manager.remove_unix_socket_on_create = false

   Default:
     manager.remove_unix_socket_on_create = true

: manager.remove_unix_socket_on_close

   Specifies whether remove used UNIX domain socket after
   shutting down. This is used only when UNIX domain socket
   is specified for manager.connection_spec.

   True if remove, false otherwise.

   Example:
     manager.remove_unix_socket_on_close = false

   Default:
     manager.remove_unix_socket_on_close = true

: manager.daemon

   Specifies whether run as daemon process or not. This item
   doesn't need to set in configuration file because this
   item can be overridden by --daemon command line option.

   True if run as daemon process, false otherwise.

   Example:
     manager.daemon = true

   Default:
     manager.daemon = false

: manager.pid_file

   Specifies file name where milter-manager saves its
   process ID.

   If security.effective_user and/or
   security.effective_group are specified, the file is wrote
   with the authority. You should take care permission of
   the file.

   File name is specified like
   "/var/run/milter/milter-manager.pid". File name should be
   surrounded with '"' (double quote). If you don't want to
   specify file name, use nil.

   Example:
     manager.pid_file = "/var/run/milter/milter-manager.pid"

   Default:
     manager.pid_file = nil

: manager.maintenance_interval

   Specifies maintenance process is ran after each N sessions.

   For now, the maintenance process is memory release
   process.

   It's a good choice that maintenance process is ran
   after each few sessions on few concurrently access
   environment. The configuration will keep memory usage
   low.

   It's a good choice that maintenance process is ran
   after each many sessions on many concurrently access
   environment. The configuration will keep process
   efficiency.

   0 or nil means maintenance process is never ran.

   Example:
     manager.maintenance_interval = nil

   Default:
     manager.maintenance_interval = 10

: manager.suspend_time_on_unacceptable

   Specifies how many seconds are suspended on
   milter-manager can't accept a connection from MTA. In the
   case, milter-manager has many connections. Please
   consider that you increment number of openable file
   descriptors by ulimit or limit in the case.

   Example:
     manager.suspend_time_on_unacceptable = 10
   Default:
     manager.suspend_time_on_unacceptable = 5

: manager.max_connections

   Since 1.3.1.

   Specifies max concurrent connections. 0 means no
   limit. It's the default value.

   A newly connection is waited until an existing connection
   is closed when there are max concurrent
   connections. Number of the current existing connections
   are confirmed each
   ((<manager.suspend_time_on_unacceptable|.#manager.suspend_time_on_unacceptable>))
   seconds.

   Example:
     manager.max_connections = 10 # accepts only 10 connections concurrency

   Default:
     manager.max_connections = 0 # no limit

: manager.max_file_descriptors

   Since 1.3.1.

   Specifies max number of file descriptors that can be
   opened by a milter-manager process. 0 means that the
   system default is used. The system is used by default
   because the default value is 0.

   milter-manager opens "number of child milters + 1 (for
   MTA)" file descriptors for each
   connection. milter-manager also opens a few file
   descriptors for its internal usage. milter-manager should
   be able to open number of file descriptors computed by
   the following expression:
     (number of child milters + 1) * max concurrent connections +
       10（milter-manager internal usage + alpha）

   This value is used as soft limit and hard limit by setrlimit(2).

   Example:
     manager.max_file_descriptors = 65535

   Default:
     manager.max_file_descriptors = 0

: manager.custom_configuration_directory

   Specifies a directory to save custom configuration via Web
   interface.

   Directory name is specified like
   "/tmp/milter-manager". Directory name should be
   surrounded with '"' (double quote).

   If you specify 'nil', milter-manager creates
   ".milter-manager" directory under effective user's home
   directory and use it for custom configuration directory.

   Example:
     manager.custom_configuration_directory = "/tmp/milter-manager/"

   Default:
     manager.custom_configuration_directory = nil

: manager.fallback_status

   Since 1.6.3.

   Specifies a status that is replied to the SMTP server on
   error in milter manager. Here is a list of error cases in
   milter manager:

     * No child milter.
     * Failed to open a temporary file for mail body.
     * and so on...

   Here are available values:

     * "accept": Accepts a mail. It's the default.
     * "temporary-failure": Rejects a mail temporary.
     * "reject": Rejects a mail.
     * "discard": Discards a mail.

   Example:
     manager.fallback_status = "reject"

   Default:
     manager.fallback_status = "accept"

: manager.fallback_status_at_disconnect

   Since 1.6.3.

   Specifies a status that is replied to the SMTP server when
   milter manager detects the SMTP client disconnects the
   SMTP server connection. This item doesn't used by default
   because disconnect check is disabled by default. You
   can enable disconnect check by
   ((<manager.use_netstat_connection_checker|.#manager.use_netstat_connection-checker>)).

   Here are available values:

     * "accept": Accepts a mail.
     * "temporary-failure": Rejects a mail temporary. It's the default.
     * "reject": Rejects a mail.
     * "discard": Discards a mail.

   Example:
     manager.fallback_status_at_disconnect = "discard"

   Default:
     manager.fallback_status_at_disconnect = "temporary-failure"

: manager.event_loop_backend

   ((*Normally, this item doesn't need to be used.*))

   Since 1.6.3.

   Specifies a backend for event loop. For non-large
   mail system which processes 100 or less mails per second,
   you don't need to change it. For large mail system which
   processes 100 or more mails per seconds, you need to
   change it to "libev".

   Here are availble values:

     * "glib": Uses GLib's event loop that uses poll(2) as
       I/O multiplexer. It's the default.
     * "libev": Uses libev that uses epoll, kqueue or event
       ports as I/O multiplexer.

   Example:
     manager.event_loop_backend = "libev"

   Default:
     manager.event_loop_backend = "glib"

: manager.n_workers

   ((*Normally, this item doesn't need to be used.*))

   Since 1.6.3.

   Specifies the number of processes which process mails. For non-large
   mail system which processes 100 or less mails per second, and mail
   system which doesn't use very heavy milter,
   you don't need to change it. For large mail system which
   processes 100 or more mails per seconds using very heavy milters,
   you need to increase it.

   Availble value is between 0 and 1000.
   If it is 0, no worker processes will be used.

   Example:
     manager.n_workers = 10

   Default:
     manager.n_workers = 0 # no worker processes.

: manager.packet_buffer_size

   ((*Normally, this item doesn't need to be used.*))

   Since 1.6.3.

   Specifies buffer size to buffer send packets on
   end-of-message. Packets aren't sent until amount of
   buffered packets is greater than buffer size. 0 means
   buffering is disabled.

   It may improve performance when many message modification
   operations, add_header, delete_recipient and so on, are
   requested on end-of-message. Normary, this configuration
   doesn't improve performance.

   Example:
     manager.packet_buffer_size = 4096 # Doesn't send packets
                                       # until amount of
                                       # buffered packets >= 4KB.

   Default:
     manager.packet_buffer_size = 0 # Disables buffering.

: manager.chunk_size

   ((*Normally, this item doesn't need to be used.*))

   Since 1.8.0.

   Specifies chunk size on body data for 2..n child milters.
   The maximum size is 65535 bytes and it is the default. If
   chunk size is decreased, communication overhead is
   incrased. You should use it only if you want to decrease
   each data size.

   Example:
     manager.chunk_size = 4096 # Sends body data as 4KB chunks.

   Default:
     manager.chunk_size = 65535 # Sends body data as 64KB chunks.

: manager.max_pending_finished_sessions

   ((*Normally, this item doesn't need to be used.*))

   Since 1.8.6.

   Milter manager delays processings that don't effect to throughput
   until idle time. For example, termination processing for finished
   milter session is one of those delayed processings.

   Milter manager does termination processing for finished milter
   sessions by setting this item. Normally, there is idle time even
   when milter manager processes multiple milter sessions concurrently.
   If the number of processing milter sssions is too large, there will
   be no idle time. So termination processins are not done. If
   termination processings are not done for a long time, the number of
   openable file descripters may be lacked. Because used socket is
   closed in termination processing.

   Normally, you should avoid the no idle time situation because it is
   overload. It is better that increasing the number of workers by
   ((<manager.n_workers|.#manager.n-workers>)) configuration.

   Milter manager does termination processing even when there is no
   idle time by setting one or larger number. Termination processing
   is done when specified the number of finished sessions. This item
   doesn't disable termination processing on idle time. So this item
   doesn't effect to throughput on normal time. This item effects
   on only no idle time.

   The default value is 0. It distables termination processing on no
   idle time feature.

   Example:
     # Do termination processing after each session is finished
     manager.max_pending_finished_sessions = 1

   Default:
     # Do termination processing when no other processings aren't remining
     manager.max_pending_finished_sessions = 0

: manager.use_netstat_connection_checker

   Since 1.5.0.

   Checks SMTP client and SMTP server are still connected by
   parsing ((%netstat%)) command result.

   This feature is useful for aborting SMTP session when
   SMTP client disconnects SMTP session. e.g. Using
   ((<taRgrey|URL:http://k2net.hakuba.jp/targrey/index.en.html>))
   with milter (milter-greylist). This feature resolves a
   problem of taRgrey that SMTP server process is
   grown. SMTP server process growth means memory usage
   growth. i.e. This feature reduces memory usage caused by
   taRgrey.

   SMTP session is checked in 5 seconds. The interval time
   can be changed but it's not needed normally.

   Example:
     manager.use_netstat_connection_checker    # check in 5 seconds.
     manager.use_netstat_connection_checker(1) # check in 1 seconds.

   Default:
     Don't check.

: manager.connection_check_interval

   ((*Normally, this item doesn't need to be used directly.*))

   Since 1.5.0.

   Specifies an interval in seconds to check whether a
   connection between SMTP client and SMTP server is still
   connected.

   0 means 'no check'.

   ((<manager.define_connection_checker|.#manager.define_connection_checker>))
   defines how to check whether a connection is still
   connected.

   Example:
     manager.connection_check_interval = 5 # Check in 5 seconds.

   Default:
     manager.connection_check_interval = 0

: manager.define_connection_checker(name) {|context| ... # -> true/false}

   ((*Normally, this item doesn't need to be used directly.*))

   Since 1.5.0.

   Checks a SMTP client is still connected to a SMTP
   server each
   ((<manager.connection_check_interval|.#manager.connection_check_interval>))
   seconds. If given block returns true value, it means that
   the connection is still alive, otherwise it means that
   the connection had been closed.

   : name
      The name of the check process.

   : context
      The object passed to the block that knows the current
      situation. It has the following information:

   : context.smtp_client_address
      Is the IP address of the check target SMTP client.
      It is the same object as
      ((<socket_address|.#socket-address>)).

   : context.smtp_server_address
      Is the IP address of the accepted SMTP server socket.
      It is the same object as
      ((<socket_address|.#socket-address>)).

   Example:
     # It assumes that a connection from non local network
     # is always closed.
     manager.define_connection_checker("netstat-check") do |context|
       context.smtp_client_address.local?
     end

: manager.report_memory_statistics

   Since 1.5.0.

   Logs memory usage each maintenance process.

   Here is the output format but it may be changed in the
   feature:

     Mar 28 15:16:58 mail milter-manager[19026]: [statistics] [maintain][memory] (28048KB) total:6979 Proc:44 GLib::Object:18

   Example:
     manager.report_memory_statistics

: manager.maintained {...}

   ((*Normally, this item doesn't need to be used directly.*))

   Since 1.5.0.

   Executes a custom process each maintenance process.

   Here is an example that logs a message each maintenance
   process.

   Example:
     manager.maintained do
       Milter::Logger.info("maintained!")
     end

: manager.event_loop_created {|loop| ...}

   ((*Normally, this item doesn't need to be used directly.*))

   Since 1.6.8.

   Executes a custom process on an event loop is created. An
   event loop is created only on initialization.

   Here is an example that registers a callback that logs a
   message at intervals of 1 second.

   Example:
     manager.event_loop_created do |loop|
       loop.add_timeout(1) do
         Milter::Logger.info("timeout!")
         true
       end
     end

== [controller] Controller

: controller.connection_spec

   Specifies a socket that milter-manager accepts connection
   for controlling milter-manager.

   Format is same as manager.connection_spec.

   Example:
     controller.connection_spec = "inet:10026@localhost"

   Default:
     controller.connection_spec = nil

: controller.unix_socket_mode

   Specifies permission of UNIX domain socket for
   controlling milter-manager. This is used only when UNIX
   domain socket is specified for controller.connection_spec.

   Don't forget to prepend '0' to specify permission as
   octal notation.

   Example:
     controller.unix_socket_mode = 0600

   Default:
     controller.unix_socket_mode = 0660

: controller.remove_unix_socket_on_create

   Specifies whether remove existing UNIX domain socket
   for controlling milter-manager before creating new UNIX
   domain socket or not. This is used only when UNIX domain
   socket is specified for controller.connection_spec.

   True if remove, false otherwise.

   Example:
     controller.remove_unix_socket_on_create = false

   Default:
     controller.remove_unix_socket_on_create = true

: controller.remove_unix_socket_on_close

   Specifies whether remove used UNIX domain socket for
   controlling milter-manager after shutting down. This is
   used only when UNIX domain socket is specified for
   controller.connection_spec.

   True if remove, false otherwise.

   Example:
     controller.remove_unix_socket_on_close = false

   Default:
     controller.remove_unix_socket_on_close = true

== [child-milter] Child milter

This section describes about configuration items related
with child milter.

=== Define child milter

Child milter is registered as the following syntax:

  define_milter("NAME") do |milter|
    milter.XXX = ...
    milter.YYY = ...
    milter.ZZZ = ...
  end

For example, to register a milter that accepts connection at
'inet:10026@localhost' as 'test-milter':

  define_milter("test-milter") do |milter|
    milter.connection_spec = "inet:10026@localhost"
  end

The following items can be used in 'define_milter do ... end'.

Required item is just only milter.connection_spec.


: milter.connection_spec

   Specifies socket that the child milter accepts.
   This is ((*required item*)).

   Format is same as manager.connection_spec.

   Example:
     milter.connection_spec = "inet:10026@localhost"

   Default:
     milter.connection_spec = nil

: milter.description

   Specifies description of the child milter.

   Description is specified like "test milter". Description is
   surrounded with '"' (double quote).

   Example:
     milter.description = "test milter"

   Default:
     milter.description = nil

: milter.enabled

   Whether use the child milter or not.

   True if use, false otherwise.

   Example:
     milter.enabled = false

   Default:
     milter.enabled = true

: milter.fallback_status

   Specifies a status that is used the child milter causes
   an error.

   Here are available values:

     * "accept": Accepts a mail. It's the default.
     * "temporary-failure": Rejects a mail temporary.
     * "reject": Rejects a mail.
     * "discard": Discards a mail.

   Example:
     milter.fallback_status = "temporary-failure"

   Default:
     milter.fallback_status = "accept"

: milter.evaluation_mode

   Since 1.3.1.

   Whether turn on evaluation mode or not. The child milter
   doesn't return its result on evaluation mode. It means
   the child milter doesn't affect the existing mail system.

   Graphs are still generated on evaluation mode because
   statistics are logged.

   True if evaluation mode is turned on, false otherwise.

   On false (default) case, message handling in milter
   session is aborted when a child milter returns "reject"
   because milter manager returns "reject" to MTA. On true
   case, message handling in milter session is continued
   when a child milter returns "reject" because milter
   manager doesn't return "reject" to MTA. You can use a
   condition that "a child milter returns 'reject'" in
   applicable conditions.

   Example:
     milter.evaluation_mode = true

   Default:
     milter.evaluation_mode = false

: milter.applicable_conditions

   Specifies applicable conditions for the child milter. The
   child milter is stopped if any condition isn't satisfied.

   The following command shows available applicable
   conditions:

     % /usr/local/sbin/milter-manager --show-config | grep define_applicable_condition
     define_applicable_condition("S25R") do |condition|
     define_applicable_condition("Remote Network") do |condition|

   In the above case, "S25R" and "Remote Network" are
   available.

   Some applicable conditions are available in default
   configuration. You can also define your original
   applicable condition. See
   ((<Define applicable condition|.#applicable-condition>))
   about how to define applicable condition. But defining
   applicable condition requires Ruby's knowledge.

   To specify multiple applicable conditions, you can use
   ",":

     milter.applicable_conditions = ["S25R", "Remote Network"]

   Example:
     milter.applicable_conditions = ["S25R"]

   Default:
     milter.applicable_conditions = []

: milter.add_applicable_condition(name)

   Adds an applicable condition to the child milter. See
   milter.applicable_conditions about 'applicable condition'.

   Example:
     milter.add_applicable_condition("S25R")

: milter.command

   Specifies command that run the child milter.
   The child milter is ran by the command automatically when
   connecting milter.connection_spec is failed. It is
   enabled only if security.privilege_mode is true and
   milter-manager is ran.

   Command may be run script that is placed in
   /etc/init.d/ or /usr/local/etc/rc.d/.

   Command is specified like
   "/etc/init.d/milter-greylist". Command should be surrounded
   with '"' (double quote). If you don't want to run
   automatically, use nil.

   Example:
     milter.command = "/etc/init.d/milter-greylist"

   Default:
     milter.command = nil

: milter.command_options

   Specifies options to be passed to milter.command.

   Options are specified like "start". Options should be
   surrounded with '"' (double quote).  If some options are
   specified, use "--option1 --option2" or ["--option1",
   "--option2"].

   Example:
     milter.command_options = "start"
     milter.command_options = ["--option1", "--option2"]

   Default:
     milter.command_options = nil

: milter.user_name

   Specifies user name to run milter.command.

   User name is specified like "nobody". User name should be
   surrounded with '"' (double quote). If you want to run
   milter.command as root, use nil.

   Example:
     milter.user_name = "nobody"

   Default:
     milter.user_name = nil

: milter.connection_timeout

   Specifies timeout in seconds for trying to connect to
   child milter.

   Example:
     milter.connection_timeout = 60

   Default:
     milter.connection_timeout = 297.0

: milter.writing_timeout

   Specifies timeout in seconds for trying to write to
   child milter.

   Example:
     milter.writing_timeout = 15

   Default:
     milter.writing_timeout = 7.0

: milter.reading_timeout

   Specifies timeout in seconds for trying to read from
   child milter.

   Example:
     milter.reading_timeout = 15

   Default:
     milter.reading_timeout = 7.0

: milter.end_of_message_timeout

   Specifies timeout in seconds for trying to wait response
   of xxfi_eom() from child milter.

   Example:
     milter.end_of_message_timeout = 60

   Default:
     milter.end_of_message_timeout = 297.0

: milter.name

  Since 1.8.1.

  Returns child milter's name specified by define_milter.

=== Operate child milter

There is convenient features to operate defined child
milters. But the features require knowledge about Ruby a
bit.

defined_milters returns a list of define child milter names.

  define_milter("milter1") do |milter|
    ...
  end

  define_milter("milter2") do |milter|
    ...
  end

  defined_milters # => ["milter1", "milter2"]

It's easy that changing configuration of all child milter by
this feature.

Here is an example that disabling all child milters:

  defined_milters.each do |name|
    define_milter(name) do |milter|
      milter.enabled = false
    end
  end

Here is an example that removing all child milters:

  defined_milters.each do |name|
    remove_milter(name)
  end

Removing differs from disabling. You need to redefine a
milter again when you want to use a removed milter again.

Here is an example that adding "S25R" applicable condition
to all child milters.

  defined_milters.each do |name|
    define_milter(name) do |milter|
      milter.add_applicable_condition("S25R")
    end
  end


: defined_milters
   Returns a list of defined child milter names. Returned
   value is an array of string.

   Example:
     defined_milters # => ["milter1", "milter2"]

: remove_milter(name)
   Removes a child milter that is named as 'name'. You
   should use ((<milter.enabled|.#milter.enabled>)) when you
   may reuse the child milter.

   Example:
     # Removes a milter that is defined as "milter1".
     remove_milter("milter1")

== [built-in-applicable-conditions] Built-in applicable conditions

Here are descriptions about built-in applicable conditions.

=== S25R

This applicable condition applies a child milter to only
normal PC like SMTP client. A child milter isn't applied to
MTA like SMTP client.

Here is an example that uses
((<Rgrey|URL:http://lists.ee.ethz.ch/postgrey/msg01214.html>))
technique. (NOTE: milter-greylist should have "racl greylist
default" configuration.)

Example:
  define_milter("milter-greylist") do |milter|
    milter.add_applicable_condition("S25R")
  end

See ((<S25R|URL:http://www.gabacho-net.jp/en/anti-spam/>))
how to determine a SMTP client is MTA or normal PC.

S25R rules will also match non normal PC host name. To avoid
this false positive, you can create whitelist. google.com
domain and obsmtp.com domain are in whitelist by default.

You can also create blacklist for normal PC host name that
isn't matched to S25R rules.

You can customize S25R applicable condition by the following
configurations:

: s25r.add_whitelist(matcher)

   Since 1.5.2.

   S25R applicable condition treats a host name that
   ((|matcher|)) matches the host name as MTA host and adds
   the host name to whitelist. If a
   host name is listed in whitelist, child milter isn't
   applied.

   ((|matcher|)) is a regular expression or a host name as
   string.

   For example, the following configuration adds google.com
   domain to whitelist:

     s25r.add_whitelist(/\.google\.com\z/)

   The following configuration adds mx.example.com host to
   whitelist:

     s25r.add_whitelist("mx.example.com")

   [For power user] You can specify complex condition by
   block. S25R applicable condition passes a host name to
   the block. For example, the following configuration adds
   .jp top level domain while 8:00 a.m. to 7:59 p.m. to
   whitelist:

     s25r.add_whitelist do |host|
       (8..19).include?(Time.now.hour) and /\.jp\z/ === host
     end

: s25r.add_blacklist(matcher)

   Since 1.5.2.

   S25R applicable condition treats a host name that
   ((|matcher|)) matches the host name as normal PC host and
   adds the host to blacklist. If a host name is listed in
   blacklist, child milter is applied.

   NOTE: If a host is listed both whitelist and blacklist,
   S25R applicable condition give preference to whitelist
   over blacklist. That is, child milter isn't applied in
   the case.

   ((|matcher|)) is a regular expression or a host name as
   string.

   For example, the following configuration adds
   evil.example.com domain to blacklist:

     s25r.add_blacklist(/\.evil\.example\.com\z/)

   The following configuration adds black.example.com host to
   blacklist:

     s25r.add_blacklist("black.example.com")

   [For power user] You can specify complex condition by
   block. S25R applicable condition passes a host name to
   the block. For example, the following configuration adds
   .jp top level domain while 8:00 p.m. to 7:59 a.m. to
   blacklist:

     s25r.add_blacklist do |host|
       !(8..19).include?(Time.now.hour) and /\.jp\z/ === host
     end

: s25r.check_only_ipv4=(boolean)

   Since 1.6.6.

   If (({true})) is specified, S25R check is enabled only
   for IPv4 connection. If (({false})) is specified, S25R
   check is also enabled for IPv6 connection.

   Example:
     s25r.check_only_ipv4 = false # enabled for non IPv4 connection

   Default:
     S25R check is enabled only for IPv4.

=== Remote Network

This applicable condition applies a child milter to only
SMTP client that is connected from remote network.

Here is an example that mails from local network are skipped
spam-check to avoid false detection:

Example:
  define_milter("spamass-milter") do |milter|
    milter.add_applicable_condition("Remote Network")
  end

Local network means that not private IP
address. e.g. 192.168.0.0/24. You can customize local network
by the following configurations.

: remote_network.add_local_address(address)

   Since 1.5.0.

   Adds the specified IPv4/IPv6 address or IPv4/IPv6 network
   to local network. Child milter isn't applied to SMTP
   clients connected from local network.

   Example:

     # Don't apply child milters connections from 160.29.167.10.
     remote_network.add_local_address("160.29.167.10")
     # Don't apply child milters connections from
     # 160.29.167.0/24 network.
     remote_network.add_local_address("160.29.167.0/24")
     # Don't apply child milters connections from
     # 2001:2f8:c2:201::fff0.
     remote_network.add_local_address("2001:2f8:c2:201::fff0")
     # Don't apply child milters connections from
     # 2001:2f8:c2:201::/64 network.
     remote_network.add_local_address("2001:2f8:c2:201::/64")

=== [authentication] Authentication

This applicable condition applies a child milter to
authenticated SMTP client by SMTP Auth. MTA should send
authentication related macros to a milter. Sendmail isn't
needed additional configuration. Postfix needs the following
additional configuration:

main.cf:
  milter_mail_macros = {auth_author} {auth_type} {auth_authen}

Here is an example that authenticated SMTP client's mail,
inner to outer mail, is Bcc-ed to be audited:

Example:

  define_milter("milter-bcc") do |milter|
    milter.add_applicable_condition("Authentication")
  end

=== [unauthentication] Unauthentication

This applicable condition applies a child miter to
non-authenticated SMTP client by SMTP Auth. MTA should send
authentication related macros to a milter. See also
((<Authentication|.#authentication>)).

Here is an example that only non-authentication SMTP client
is applied spam-check to avoid false detection.

Example:

  define_milter("spamass-milter") do |milter|
    milter.add_applicable_condition("Unauthentication")
  end

=== Sendmail Compatible

This applicable condition is a bit strange. This always
applies a child milter. This substitute different macros
between Sendmail and Postfix. A milter that depends on
Sendmail specific macros can be worked with this applicable
condition and Postfix.

This applicable condition will be needless in the near
feature because milters are fixing to be work with both
Sendmail and Postfix. It's good things.

You can use this applicable condition with Postfix, it
doesn't have adverse affect for Postfix.

Here is an example that you use milter-greylist built for
Sendmail with Postfix.

Example:

  define_milter("milter-greylist") do |milter|
    milter.add_applicable_condition("Sendmail Compatible")
  end

=== stress

Since 1.5.0.

Those applicable conditions changes process depends on
stress dynamically. Stress is determine by number of
concurrent connections.

: stress.threshold_n_connections

   Since 1.5.0.

   Returns number of concurrent connections to determine
   stressed.

   With Postfix, number of max smtpd processes are detected
   automatically and 3/4 of it is set.

   With Sendmail, it's not detected automatically. You need
   to set it with
   ((<stress.threshold_n_connections=|.#stress.threshold_n_connections=>))
   by hand.

   Example:
     # Postfix's default. (depends on our environment)
     stress.threshold_n_connections # => 75

: stress.threshold_n_connections=(n)

   Since 1.5.0.

   Sets number of connections to determine stressed.

   0 means that always non-stressed.

   Example:
     # Number of concurrent connections is higher or equal
     # 75 means stressed.
     stress.threshold_n_connections = 75

==== [no-stress] No Stress

Since 1.5.0.

This applies a child milter only when non-stressed.

Here is an example that spamass-milter isn't applied when
stressed:

Example:

  define_milter("spamass-milter") do |milter|
    milter.add_applicable_condition("No Stress")
  end

==== [stress-notify] Stress Notify

Since 1.5.0.

This notifies stressed to a child milter by "{stress}=yes"
macro. This just notifies. It means that a child milter is
always applied.

Here is an example that milter-greylist is notified stressed
by macro.

Example:

  define_milter("milter-greylist") do |milter|
    milter.add_applicable_condition("Stress Notify")
  end

Here is an example configuration for milter-greylist to use
tarpitting on non-stress and greylisting on stress. This
configuration requires milter-greylist 4.3.4 or later.

greylist.conf:
  sm_macro "no_stress" "{stress}" unset
  racl whitelist sm_macro "no_stress" tarpit 125s
  racl greylist default

=== [trust] Trust

Since 1.6.0.

This sets "trusted_XXX=yes" macros for trusted session. Here
is a list of macros:

: trusted_domain

   This is set to "yes" when envelope-from domain is trusted.

Here is an example that you receive a SPF passed mail but
apply greylist SPF not-passed mail for trusted domains:

milter-manager.local.conf:

  define_milter("milter-greylist") do |milter|
    milter.add_applicable_condition("Trust")
  end

greylist.conf:

  sm_macro "trusted_domain" "{trusted_domain}" "yes"
  racl whitelist sm_macro "trusted_domain" spf pass
  racl greylist sm_macro "trusted_domain" not spf pass

You can customize how to trust a session by the following
configurations:

: trust.add_envelope_from_domain(domain)

   Since 1.6.0.

   This adds a trusted envelope-from domain.

   Here is a list of trusted domain by default:

     * gmail.com
     * hotmail.com
     * msn.com
     * yahoo.co.jp
     * softbank.ne.jp
     * clear-code.com

   Example:
     trust.add_envelope_from_domain("example.com")

: trust.clear

    Since 1.8.0.

    This clears all registered trusted envelope-from domain list.

    Example:
      trust.clear

: trust.load_envelope_from_domains(path)

    Since 1.8.0.

    This loads trusted envelope-from domain list from
    ((|path|)). Here is ((|path|)) format:

      # This is comment line. This line is ignored.
      gmail.com
      # The above line means 'gmail.com is trusted domain'.
      /\.example\.com/
      # The above line means 'all sub domains of example.com are trusted'.

      # The above line consists of spaces. The space only line is ignored.

    Example:
      trust.load_envelope_from_domains("/etc/milter-manager/trusted-domains.list")
      # It loads trusted envelope-from domain list from
      # /etc/milter-manager/trusted-domains.list.

=== Restrict Accounts

TODO

== [applicable-condition] Define applicable condition

You need knowledge about Ruby from this section. Some useful
applicable conditions are provided by default. You can define
new applicable conditions if you need more conditions. You can
decide which child milter is applied or not dynamically by
defining our applicable conditions.

You define an applicable condition with the following
syntax. You need knowledge about Ruby for defining applicable
condition.

  define_applicable_condition("NAME") do |condition|
    condition.description = ...
    condition.define_connect_stopper do |...|
      ...
    end
    ...
  end

For example, here is an applicable condition to implement
S25R:

  define_applicable_condition("S25R") do |condition|
    condition.description = "Selective SMTP Rejection"

    condition.define_connect_stopper do |context, host, socket_address|
      case host
      when "unknown",
        /\A\[.+\]\z/,
        /\A[^.]*\d[^\d.]+\d.*\./,
        /\A[^.]*\d{5}/,
        /\A(?:[^.]+\.)?\d[^.]*\.[^.]+\..+\.[a-z]/i,
        /\A[^.]*\d\.[^.]*\d-\d/,
        /\A[^.]*\d\.[^.]*\d\.[^.]+\..+\./,
        /\A(?:dhcp|dialup|ppp|[achrsvx]?dsl)[^.]*\d/i
        false
      else
        true
      end
    end
  end

'host' is "[IP ADDRESS]" not "unknown" when name resolution
is failed. So, "unknown" is needless. It's sufficient that
you just use /\A\[.+\]\z/. But it is included just in case. :-)

Here are configurable items in 'define_applicable_condition
do ... end'.

There is no required item.

: condition.description

   Specifies description of the applicable condition.

   Description is specified like "test condition". Description is
   surrounded with '"' (double quote).

   Example:
     condition.description = "test condition"

   Default:
     condition.description = nil

: condition.define_connect_stopper {|context, host, socket_address| ...}

   Decides whether the child milter is applied or not with
   host name and IP address of connected SMTP client. The
   available information is same as milter's xxfi_connect.

   It returns true if you stop the child milter, false otherwise.

   : context
      The object that has several information at the
      time. Details are said later.

   : host
      The host name (string) of connected SMTP client. It is
      got by resolving connected IP address. It may be "[IP
      ADDRESS]" string when name resolution is failed. For
      example, "[1.2.3.4]".

   : [] socket_address
      The object that describes connected IP
      address. Details are said later.

   Here is an example that you stop the child milter when
   SMTP client is connected from resolvable host:

     condition.define_connect_stopper do |context, host, socket_address|
       if /\A\[.+\]\z/ =~ host
         false
       else
         true
       end
     end

: condition.define_helo_stopper {|context, fqdn| ...}

   Decides whether the child milter is applied or not with
   FQDN on HELO/EHLO command. The available information is
   same as milter's xxfi_helo.

   It returns true if you stop the child milter, false otherwise.

   : context
      The object that has several information at the
      time. Details are said later.

   : fqdn
      The FQDN (string) sent by SMTP client on HELO/EHLO.

   Here is an example that you stop the child milter when
   FQDN is "localhost.localdomain".

     condition.define_helo_stopper do |context, helo|
       helo == "localhost.localdomain"
     end

: define_envelope_from_stopper {|context, from| ...}

   Decides whether the child milter is applied or not with
   envelope from address passed on MAIL FROM command of
   SMTP. The available information is same as milter's
   xxfi_envfrom.

   It returns true if you stop the child milter, false otherwise.

   : context
      The object that has several information at the
      time. Details are said later.

   : from
      The envelope from address passed on MAIL FROM command.
      For example, "<sender@example.com>".

   Here is an example that you stop the child milter when
   mails are sent from example.com.

     condition.define_envelope_from_stopper do |context, from|
       if /@example.com>\z/ =~ from
         true
       else
         false
       end
     end

: define_envelope_recipient_stopper {|context, recipient| ...}

   Decides whether the child milter is applied or not with
   envelope recipient address passed on RCPT TO command of
   SMTP. The available information is same as milter's
   xxfi_envrcpt. This callback is called one or more times
   when there are multiple recipients.

   It returns true if you stop the child milter, false otherwise.

   : context
      The object that has several information at the
      time. Details are said later.

   : recipient
      The envelope recipient address passed on RCPT TO command.
      For example, "<receiver@example.com>".

   Here is an example that you stop the child milter when
   mails are sent to ml.example.com.

     condition.define_envelope_recipient_stopper do |context, recipient|
       if /@ml.example.com>\z/ =~ recipient
         true
       else
         false
       end
     end

: condition.define_data_stopper {|context| ...}

   Decides whether the child milter is applied or not on
   DATA. The available information is same as milter's
   xxfi_data.

   It returns true if you stop the child milter, false otherwise.

   : context
      The object that has several information at the
      time. Details are said later.

   Here is an example that you stop the child milter on DATA.
   milter can only add/delete/modify header and/or body
   after whole message is processed. If you stop the child
   milter on DATA, you ensure that milter don't
   add/delete/modify header and/or body. You can confirm
   the child milter's work if the child milter logs its
   processed result.

     condition.define_data_stopper do |context|
       true
     end

: define_header_stopper {|context, name, value| ...}

   Decides whether the child milter is applied or not with
   header information. The available information is same as
   milter's xxfi_header. This callback is called for each
   header.

   It returns true if you stop the child milter, false otherwise.

   : context
      The object that has several information at the
      time. Details are said later.

   : name
      The header name. For example, "From".

   : value
      The header value. For example, "sender@example.com".

   Here is an example that you stop the child milter when
   mails have a header that name is "X-Spam-Flag" and value
   is "YES".

     condition.define_header_stopper do |context, name, value|
       if ["X-Spam-Flag", "YES"] == [name, value]
         true
       else
         false
       end
     end

: condition.define_end_of_header_stopper {|context| ...}

   Decides whether the child milter is applied or not after
   all headers are processed. The available information is
   same as milter's xxfi_eoh.

   It returns true if you stop the child milter, false otherwise.

   : context
      The object that has several information at the
      time. Details are said later.

   Here is an example that you stop the child milter after all
   headers are processed.

     condition.define_end_of_header_stopper do |context|
       true
     end

: condition.define_body_stopper {|context, chunk| ...}

   Decides whether the child milter is applied or not with
   a body chunk. The available information is same as
   milter's xxfi_body. This callback may be called multiple
   times for a large body mail.

   It returns true if you stop the child milter, false otherwise.

   : context
      The object that has several information at the
      time. Details are said later.

   Here is an example that you stop the child milter after all
   headers are processed.

   : chunk
      A chunk of body. A large body is not processed at a
      time. It is processed as small chunks. The maximum
      chunk size is 65535 byte.

   Here is an example that you stop the child milter when
   chunk contains PGP signature.

     condition.define_body_stopper do |context, chunk|
       if /^-----BEGIN PGP SIGNATURE-----$/ =~ chunk
         true
       else
         false
       end
     end

: condition.define_end_of_message_stopper {|context| ...}

   Decides whether the child milter is applied or not after
   a mail is processed. The available information is
   same as  milter's xxfi_eom.

   It returns true if you stop the child milter, false otherwise.

   : context
      The object that has several information at the
      time. Details are said later.

   Here is an example that you stop the child milter after
   a mail is processed.

     condition.define_end_of_message_stopper do |context|
       true
     end

=== context

The object that has several information when you decide
whether a child milter is applied or not. (The class of
context is Milter::Manager::ChildContext.)

It has the following information.

: context.name

   Returns the name of child milter. It's name used when
   define_milter.

   Example:
     context.name # -> "clamav-milter"

: context[name]

   Returns value of available macro in the child milter In
   libmilter API, you need to surround macro name that has
   two or more length with "{}" but it isn't
   required. context[name] works well with/without "{}".

   Example:
     context["j"] # -> "mail.example.com"
     context["rcpt_address"] # -> "receiver@example.com"
     context["{rcpt_address}"] # -> "receiver@example.com"

: context.reject?

   Returns true if the child milter returns 'reject'.
   The child milter must be enabled
   ((<milter.evaluation_mode|.#milter.evaluation_mode>)).

   Passed context should be always processing. So,
   context.reject? never return true. It's usuful when you
   use the other child milter's result. The other child can
   be retrieved by context.children[].

   Example:
     context.reject? # -> false
     context.children["milter-greylist"].reject? # -> true or false

: context.temporary_failure?

   Returns true if the child milter returns 'temporary failure'.
   The child milter must be enabled
   ((<milter.evaluation_mode|.#milter.evaluation_mode>)).

   Passed context should be always processing. So,
   context.temporay_failure? never return true. It's usuful
   when you use the other child milter's result. The other
   child can be retrieved by context.children[].

   Example:
     context.temporary_failure? # -> false
     context.children["milter-greylist"].temporary_failure? # -> true or false

: context.accept?

   Returns true if the child milter returns 'accept'.

   Passed context should be always processing. So,
   context.accept? never return true. It's usuful
   when you use the other child milter's result. The other
   child can be retrieved by context.children[].

   Example:
     context.accept? # -> false
     context.children["milter-greylist"].accept? # -> true or false

: context.discard?

   Returns true if the child milter returns 'discard'.
   The child milter must be enabled
   ((<milter.evaluation_mode|.#milter.evaluation_mode>)).

   Passed context should be always processing. So,
   context.discard? never return true. It's usuful
   when you use the other child milter's result. The other
   child can be retrieved by context.children[].

   Example:
     context.discard? # -> false
     context.children["milter-greylist"].discard? # -> true or false

: context.quitted?

   Returns true if the child milter was quitted.

   Passed context should be always processing. So,
   context.quitted? never return true. It's usuful
   when you use the other child milter's result. The other
   child can be retrieved by context.children[].

   Example:
     context.quitted? # -> false
     context.children["milter-greylist"].quitted? # -> true or false

: context.children[name]

   Returnes the other child milter's context.

   The name to refere the other child milter is a name that
   is used for define_milter. (i.e. the name returned by
   context.name)

   It returns nil if you refer with nonexistent name.

   Example:
     context.children["milter-greylist"] # -> milter-greylist's context
     context.children["nonexistent"]     # -> nil
     context.children[context.name]      # -> my context

: context.postfix?

   Returns true when MTA is Postfix. It is decided by "v"
   macro's value. If the value includes "Postfix", MTA will
   be Postfix.

   Returns true when MTA is Postfix, false otherwise.

   Example:
     context["v"]     # -> "Postfix 2.5.5"
     context.postfix? # -> true

     context["v"]     # -> "2.5.5"
     context.postfix? # -> false

     context["v"]     # -> nil
     context.postfix? # -> false

: context.authenticated?

   Returns true when sender is authenticated. It is decided by
   "auto_type" macro or "auth_authen" macro is
   available. They are available since MAIL FROM. So, it
   always returns false before MAIL FROM. You don't forget to
   add the following configuration to main.cf if you are
   using Postfix.

     milter_mail_macros = {auth_author} {auth_type} {auth_authen}

   It returns true when sender is authenticated, false otherwise.

   Example:
     context["auth_type"]   # -> nil
     context["auth_authen"] # -> nil
     context.authenticated? # -> false

     context["auth_type"]   # -> "CRAM-MD5"
     context["auth_authen"] # -> nil
     context.authenticated? # -> true

     context["auth_type"]   # -> nil
     context["auth_authen"] # -> "sender"
     context.authenticated? # -> true

=== [socket-address] socket_address

The object that describes socket address. Socket is one of
IPv4 socket, IPv6 socket and UNIX domain socket. Socket
address is described as corresponding object.

==== Milter::SocketAddress::IPv4

It describes IPv4 socket address. It has the following methods.

: address
   Returns dot-notation IPv4 address.

   Example:
     socket_address.address # -> "192.168.1.1"

: port
   Returns port number.

   Example:
     socket_address.port # -> 12345

: to_s
   Returns IPv4 address formated as connection_spec format.

   Example:
     socket_address.to_s # -> "inet:12345@[192.168.1.1]"

: local?
   Returns true if the address is private network address,
   false otherwise.

   Example:
     socket_address.to_s   # -> "inet:12345@[127.0.0.1]"
     socket_address.local? # -> true

     socket_address.to_s   # -> "inet:12345@[192.168.1.1]"
     socket_address.local? # -> true

     socket_address.to_s   # -> "inet:12345@[160.XXX.XXX.XXX]"
     socket_address.local? # -> false

: to_ip_address
   Returnes corresponding IPAddr object.

   Example:
     socket_address.to_s          # -> "inet:12345@[127.0.0.1]"
     socket_address.to_ip_address # -> #<IPAddr: IPv4:127.0.0.1/255.255.255.255>

==== Milter::SocketAddress::IPv6

It describes IPv6 socket address. It has the following methods.

: address
   Returns colon-notation IPv6 address.

   Example:
     socket_address.address # -> "::1"

: port
   Returns port number.

   Example:
     socket_address.port # -> 12345

: to_s
   Returns IPv6 address formated as connection_spec format.

   Example:
     socket_address.to_s # -> "inet6:12345@[::1]"

: local?
   Returns true if the address is private network address,
   false otherwise.

   Example:
     socket_address.to_s   # -> "inet6:12345@[::1]"
     socket_address.local? # -> true

     socket_address.to_s   # -> "inet6:12345@[fe80::XXXX]"
     socket_address.local? # -> true

     socket_address.to_s   # -> "inet6:12345@[2001::XXXX]"
     socket_address.local? # -> false

: to_ip_address
   Returnes corresponding IPAddr object.

   Example:
     socket_address.to_s          # -> "inet6:12345@[::1]"
     socket_address.to_ip_address # -> #<IPAddr: IPv6:0000:0000:0000:0000:0000:0000:0000:0001/ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff>

==== Milter::SocketAddress::Unix

It describes UNIX domain ssocket address. It has the following methods.

: path
   Returns path of the socket.

   Example:
     socket_address.path # -> "/tmp/local.sock"

: to_s
   Returns UNIX domain socket address formated as
   connection_spec format.

   Exampel:
     socket_address.to_s # -> "unix:/tmp/local.sock"

: local?
   Always returns true.

   Example:
     socket_address.local? # -> true

: to_ip_address
   Always returnes nil.

   Example:
     socket_address.to_s          # -> "unix:/tmp/local.sock"
     socket_address.to_ip_address # -> nil

== [database] Database

Since 1.6.6.

You need knowledge about Ruby for this section. Milter
manager supports
((<ActiveRecord|URL:http://api.rubyonrails.org/files/activerecord/README_rdoc.html>))
as database operation library. You can use many RDB like
MySQL, SQLite3 and so on because ActiveRecord supports them.

You need to install ActiveRecord to use database. See
optional ((<install|install-to>)) documents for ActiveRecord
install.

Let's start small example for using 'users' table in MySQL.

Here is connection information for MySQL server:

: Database name
   mail-system

: IP address of database server
   192.168.0.1

: User name
   milter-manager

: Password
   secret

First, you write the above connection information to
milter-manager.local.conf. In this example,
milter-manager.local.conf is placed at
/etc/milter-manager/milter-manager.local.conf.

/etc/milter-manager/milter-manager.local.conf:
  database.type = "mysql2"
  database.name = "mail-system"
  database.user = "milter-manager"
  database.password = "secret"

Next, you define ActiveRecord object to operate 'users'
table. Definition files are placed at models/
directory. models/ directory is placed at the same directory
of milter-manager.local.conf. You create models/user.rb.

/etc/milter-manager/models/user.rb:
  class User < ActiveRecord::Base
  end

Last, you connect to database and operate data in
milter-manager.local.conf:

/etc/milter-manager/milter-manager.local.conf:
  database.setup
  database.load_models("models/*.rb")
  User.all.each do |user|
    p user.name # => "alice", "bob", ...
  end

Here are completed files:

/etc/milter-manager/milter-manager.local.conf:
  # Configure connection information
  database.type = "mysql2"
  database.name = "mail-system"
  database.user = "milter-manager"
  database.password = "secret"

  # Connect
  database.setup

  # Load definitions
  database.load_models("models/*.rb")
  # Operate data
  User.all.each do |user|
    p user.name # => "alice", "bob", ...
  end

/etc/milter-manager/models/user.rb:
  class User < ActiveRecord::Base
  end

Here are configuration items:

: database.type
   Specifies a database type.

   Here are available types:
     : "mysql2"
        Uses MySQL. You need to install mysql2 gem:

          % sudo gem install mysql2

     : "sqlite3"
        Uses SQLite3. You need to install sqlite3 gem:

          % sudo gem install sqlite3

     : "pg"
        Uses PostgreSQL. You need to install pg gem:

          % sudo gem install pg

   Example:
     database.type = "mysql2" # uses MySQL

: database.name
   Specifies database name.

   For SQLite3, it specifies database path or (({":memory:"})).

   Example:
     database.name = "configurations" # connects 'configurations' database

: database.host
   Specifies database server host name.

   In MySQL and so on, "localhost" is used as the default value.

   In SQLite3, it is ignored.

   Example:
     database.host = "192.168.0.1" # connects to server running at 192.168.0.1

: database.port
   Specifies database server port number.

   In many cases, you don't need to specify this value
   explicitly. Because an applicable default value is used.

   In SQLite3, it is ignored.

   Example:
     database.port = 3306 # connects to server running at3 3306 port.

: database.path
   Specifies database server UNIX domain socket path.

   In SQLite3, it is used as database path. But
   ((<.#database.name>)) is prioritize over this. It's
   recommended that ((<.#database.name>)) is used rather than
   ((<.#database.path>)).

   Example:
     database.path = "/var/run/mysqld/mysqld.sock" # connects to MySQL via UNIX domain socket

: database.user
   Specifies database user on connect.

   In SQLite3, it is ignored.

   Example:
     database.user = "milter-manager" # connects to server as 'milter-manager' user

: database.password
   Specifies database password on connect.

   In SQLite3, it is ignored.

   Example:
     database.password = "secret"


: database.extra_options
   Since 1.6.9.

   Specifies extra options. Here is an example to specify
   (({:reconnect})) option to ActiveRecord's MySQL2 adapter:

     database.type = "mysql2"
     database.extra_options[:reconnect] = true

   Available extra options are different for each database.

   Example:
     database.extra_options[:reconnect] = true

: database.setup
   Connects to database.

   You connect to database at this time. After this, you can
   operate data.

   Example:
     database.setup

: database.load_models(path)
   Loads Ruby scripts that write class definitions for
   ActiveRecord. You can use glob for ((|path|)). You can
   use "models/*.rb" for loading all files under models/
   directory. If ((|path|)) is relative path, it is resolved
   from a directory that has milter-manager.conf.

   Example:
     # Loads
     #   /etc/milter-manager/models/user.rb
     #   /etc/milter-manager/models/group.rb
     #   /etc/milter-manager/models/...
     # (In /etc/milter-manager/milter-manager.conf case.)
     database.load_models("models/*.rb")
