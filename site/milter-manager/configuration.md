---
title: Configuration
---

# Configuration --- How to write milter-manager.conf

## About this document

This document describes how to write milter-manager.confthat is milter-manager's configuration file.

## Place

It assumes that milter-manager is installed under/usr/local/. If you specify --prefix=/usr/local option toconfigure script or omit the option, milter-manager isinstalled under /usr/local/.

In this case, milter-manager's configuration file is placedat /usr/local/etc/milter-manager/milter-manager.conf.If you have installed successfully, the file should exist.

## Summary

The beginning milter-manager.conf is the following:

<pre># -*- ruby -*-

load("applicable-conditions/*.conf")
load_default
load_if_exist("milter-manager.local.conf")</pre>

Normally, the part should not be changed. If you need tochange your configuration, you should write yourconfigurations to "milter-manager.local.conf" file in thesame directory of "milter-manager.conf".

Configuration items are categorized as the followings:

* Package
* Security
* milter-manager
* Controller
* Built-in applicable conditions
* Applicable condition
* Child milter
* Database

There is a convenient feature of milter-manager. It'sintroduced before each items are described the below.

If you run milter-manager with --show-config option, thecurrent configuration is shown.

<pre>% /usr/local/sbin/milter-manager --show-config
package.platform = "debian"
package.options = nil

security.privilege_mode = false
security.effective_user = nil
security.effective_group = nil

log.level = "default"
log.path = nil
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
end</pre>

You can confirm the current configuration.

The output format is the same as milter-manager.conf'ssyntax. You can refer the output for writingmilter-manager.conf.

Here are descriptions of configuration items.

## Package {#package}

<dl>
<dt>package.platform</dt>
<dd><em>Normally, this item doesn't need to be changed.</em>

milter auto-detect method is different on eachplatform. The auto-detect method assumes that milters areinstalled with package system on its platform. Ifreal platform and platform detected by milter-manager aredifferent, the auto-detect method doesn't work well.

Your platform is detected on building milter-manager. Youcan specify correct platform if the detected platform iswrong. You will use this item only when the detectedplatform is wrong but you can't build again.

Here are supported platforms:

* debian: for Debian series Linux like Debian       GNU/Linux and Ubuntu Linux.
* redhat: for RedHat series Linux like CentOS.
* freebsd: for FreeBSD.
* pkgsrc: for *BSD that use pkgsrc like NetBSD and       DragonFly BSD.

Platform name should be surrounded with '"' (double quote)like "debian".

NOTE: This item should be changed <em>before</em> load_default.

Example:

<pre>package.platform = "pkgsrc"</pre>

Default:

<pre>package.platform = "debian" # depend on your environment.</pre></dd>
<dt>package.options</dt>
<dd><em>Normally, this item doesn't need to be changed.</em>

This item is determined on building like package.platform.

You can pass additional information to milter auto-detectmethod. The format of this item is"NAME1=VALUE1,NAME2=VALUE2,...". You can pass zero ormore information.

Currently, additional information is only used on"pkgsrc" platform. "pkgsrc" platform only uses"prefix=/PATH/TO/DIRECTORY/HAS/rc.d". For example, youneed to specify "prefix=/etc" if you install start-scriptunder /etc/rc.d/ directory.

NOTE: This item should be changed <em>before</em> load_default.

Example:

<pre>package.options = "prefix=/etc,name=value"</pre>

Default:

<pre>package.options = nil # depend on your environment.</pre></dd></dl>

## Security {#security}

<dl>
<dt>security.privilege_mode</dt>
<dd>Specifies whether run as privilege mode or not. If youwant to use child milter auto run feature, you need toenable the mode.

It's set true for enabling the mode, false otherwise.

Example:

<pre>security.privilege_mode = true</pre>

Default:

<pre>security.privilege_mode = false</pre></dd>
<dt>security.effective_user</dt>
<dd>Specifies effective user of milter-manager process. To switcheffective user, you need to run milter-manager command asroot user.

Effective user is specified like "nobody". User name shouldbe surrounded with '"' (double quote). If you don't want tospecify user, use nil.

Example:

<pre>security.effective_user = "nobody"</pre>

Default:

<pre>security.effective_user = nil</pre></dd>
<dt>security.effective_group</dt>
<dd>Specifies effective group of milter-manager process. To switcheffective group, you need to run milter-manager command asroot group.

Effective group is specified like "nogroup". Group name shouldbe surrounded with '"' (double quote). If you don't want tospecify group, use nil.

Example:

<pre>security.effective_group = "nobody"</pre>

Default:

<pre>security.effective_group = nil</pre></dd></dl>

## Log {#log}

Since 1.6.6.

<dl>
<dt>log.level</dt>
<dd>Specifies log level. All log levels are independent. Youcan specify log level by combining log levels what youneed like "info, debug and error levels are needed".

Here are available log levels:

<dl>
<dt>default</dt>
<dd>Logs critical, error, warnings, message andstatistics level messages. It's the default.</dd>
<dt>all</dt>
<dd>Logs all messages.</dd>
<dt>critical</dt>
<dd>Logs critical messages.</dd>
<dt>error</dt>
<dd>Logs error messages.</dd>
<dt>warning</dt>
<dd>Logs warnings messages.</dd>
<dt>message</dt>
<dd>Logs important messages.</dd>
<dt>info</dt>
<dd>Logs normal messages.</dd>
<dt>debug</dt>
<dd>Logs debug messages.</dd>
<dt>statistics</dt>
<dd>Logs statistics messages.</dd>
<dt>profile</dt>
<dd>Logs profile messages.</dd></dl>

Log level should be surrounded with '"' (double quote)like "all". To specify some log levels, you need toseparate each log level by "|" like"critical|error|warning".

Example:

<pre>log.level = "all"        # Logs all messages</pre>

Default:

<pre>log.level = "default"</pre></dd>
<dt>log.path</dt>
<dd>Specifies the log output file path.

If the value is nil, log is outputted to the standard output.

Example:

<pre>log.path = nil                            # Output to the standard output
log.path = "/var/log/milter-manager.log"  # Output to the file</pre>

Default:

<pre>log.path = nil</pre></dd>
<dt>log.use_syslog</dt>
<dd>Specifies whether syslog is also used.

Set true for using syslog, false otherwise.

Example:

<pre>log.use_syslog = false   # Syslog isn't used.</pre>

Default:

<pre>log.use_syslog = true</pre></dd>
<dt>log.syslog_facility</dt>
<dd>Specifies syslog facility.

Here are available facilities and corresponded syslogconstants:

<dl>
<dt>authpriv</dt>
<dd>LOG_AUTHPRIV</dd>
<dt>cron</dt>
<dd>LOG_CRON</dd>
<dt>daemon</dt>
<dd>LOG_DAEMON</dd>
<dt>kern</dt>
<dd>LOG_KERN</dd>
<dt>local0</dt>
<dd>LOG_LOCAL0</dd>
<dt>local1</dt>
<dd>LOG_LOCAL1</dd>
<dt>local2</dt>
<dd>LOG_LOCAL2</dd>
<dt>local3</dt>
<dd>LOG_LOCAL3</dd>
<dt>local4</dt>
<dd>LOG_LOCAL4</dd>
<dt>local5</dt>
<dd>LOG_LOCAL5</dd>
<dt>local6</dt>
<dd>LOG_LOCAL6</dd>
<dt>local7</dt>
<dd>LOG_LOCAL7</dd>
<dt>lpr</dt>
<dd>LOG_LPR</dd>
<dt>mail</dt>
<dd>LOG_MAIL</dd>
<dt>news</dt>
<dd>LOG_NEWS</dd>
<dt>user</dt>
<dd>LOG_USER</dd>
<dt>uucp</dt>
<dd>LOG_UUCP</dd></dl>

Facility should be surrounded with '"' (double quote)like "mail".

Example:

<pre>log.syslog_facility = "local4"   # LOG_LOCAL4 is used.</pre>

Default:

<pre>log.syslog_facility = "mail"</pre></dd></dl>

## milter-manager {#milter-manager}

<dl>
<dt>manager.connection_spec</dt>
<dd>Specifies a socket that milter-manager accepts connectionfrom MTA.

Socket is specified like "inet:10025". Socket should besurrounded with '"' (double quote). Available socketformats are the following:

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

If security.effective_user and/orsecurity.effective_group are specified, UNIX domainsocket is made with the authority.

If HOST of IPv4 socket and IPv6 socket is omitted,milter-manager accepts connection from all networkinterface. If HOST is specified, milter-manager acceptsconnection from the address.

Example:

<pre>manager.connection_spec = "unix:/var/run/milter/milter-manager.sock"</pre>

Default:

<pre>manager.connection_spec = "inet:10025@[127.0.0.1]"</pre></dd>
<dt>manager.unix_socket_mode</dt>
<dd>Specifies permission of UNIX domain socket thatmilter-manager uses for acceptingconnection. This is used only when UNIX domain socket isspecified for manager.connection_spec.

Don't forget to prepend '0' to specify permission asoctal notation.

Example:

<pre>manager.unix_socket_mode = 0600</pre>

Default:

<pre>manager.unix_socket_mode = 0660</pre></dd>
<dt>manager.unix_socket_group</dt>
<dd>Specifies group of UNIX domain socket thatmilter-manager uses for acceptingconnection. This is used only when UNIX domain socket isspecified for manager.connection_spec.

Socket's group is changed by chown(2) after creating asocket withsecurity.effective_user/security.effective_groupauthority. Specified group should be one of thesecurity.effective_user's supplementary groups.

Group is specified like "nogroup". Group name shouldbe surrounded with '"' (double quote). If you don't want tospecify group, use nil.

Example:

<pre>manager.unix_socket_group = "nobody"</pre>

Default:

<pre>manager.unix_socket_group = nil</pre></dd>
<dt>manager.remove_unix_socket_on_create</dt>
<dd>Specifies whether remove existing UNIX domain socketbefore creating new UNIX domain socket or not. This isused only when UNIX domain socket is specified formanager.connection_spec.

True if remove, false otherwise.

Example:

<pre>manager.remove_unix_socket_on_create = false</pre>

Default:

<pre>manager.remove_unix_socket_on_create = true</pre></dd>
<dt>manager.remove_unix_socket_on_close</dt>
<dd>Specifies whether remove used UNIX domain socket aftershutting down. This is used only when UNIX domain socketis specified for manager.connection_spec.

True if remove, false otherwise.

Example:

<pre>manager.remove_unix_socket_on_close = false</pre>

Default:

<pre>manager.remove_unix_socket_on_close = true</pre></dd>
<dt>manager.daemon</dt>
<dd>Specifies whether run as daemon process or not. This itemdoesn't need to set in configuration file because thisitem can be overridden by --daemon command line option.

True if run as daemon process, false otherwise.

Example:

<pre>manager.daemon = true</pre>

Default:

<pre>manager.daemon = false</pre></dd>
<dt>manager.pid_file</dt>
<dd>Specifies file name where milter-manager saves itsprocess ID.

If security.effective_user and/orsecurity.effective_group are specified, the file is wrotewith the authority. You should take care permission ofthe file.

File name is specified like"/var/run/milter/milter-manager.pid". File name should besurrounded with '"' (double quote). If you don't want tospecify file name, use nil.

Example:

<pre>manager.pid_file = "/var/run/milter/milter-manager.pid"</pre>

Default:

<pre>manager.pid_file = nil</pre></dd>
<dt>manager.maintenance_interval</dt>
<dd>Specifies maintenance process is ran after each N sessions.

For now, the maintenance process is memory releaseprocess.

It's a good choice that maintenance process is ranafter each few sessions on few concurrently accessenvironment. The configuration will keep memory usagelow.

It's a good choice that maintenance process is ranafter each many sessions on many concurrently accessenvironment. The configuration will keep processefficiency.

0 or nil means maintenance process is never ran.

Example:

<pre>manager.maintenance_interval = nil</pre>

Default:

<pre>manager.maintenance_interval = 10</pre></dd>
<dt>manager.suspend_time_on_unacceptable</dt>
<dd>Specifies how many seconds are suspended onmilter-manager can't accept a connection from MTA. In thecase, milter-manager has many connections. Pleaseconsider that you increment number of openable filedescriptors by ulimit or limit in the case.

Example:

<pre>manager.suspend_time_on_unacceptable = 10</pre>

Default:

<pre>manager.suspend_time_on_unacceptable = 5</pre></dd>
<dt>manager.max_connections</dt>
<dd>Since 1.3.1.

Specifies max concurrent connections. 0 means nolimit. It's the default value.

A newly connection is waited until an existing connectionis closed when there are max concurrentconnections. Number of the current existing connectionsare confirmed eachmanager.suspend_time_on_unacceptableseconds.

Example:

<pre>manager.max_connections = 10 # accepts only 10 connections concurrency</pre>

Default:

<pre>manager.max_connections = 0 # no limit</pre></dd>
<dt>manager.max_file_descriptors</dt>
<dd>Since 1.3.1.

Specifies max number of file descriptors that can beopened by a milter-manager process. 0 means that thesystem default is used. The system is used by defaultbecause the default value is 0.

milter-manager opens "number of child milters + 1 (forMTA)" file descriptors for eachconnection. milter-manager also opens a few filedescriptors for its internal usage. milter-manager shouldbe able to open number of file descriptors computed bythe following expression:

<pre>(number of child milters + 1) * max concurrent connections +
  10（milter-manager internal usage + alpha）</pre>

This value is used as soft limit and hard limit by setrlimit(2).

Example:

<pre>manager.max_file_descriptors = 65535</pre>

Default:

<pre>manager.max_file_descriptors = 0</pre></dd>
<dt>manager.custom_configuration_directory</dt>
<dd>Specifies a directory to save custom configuration via Webinterface.

Directory name is specified like"/tmp/milter-manager". Directory name should besurrounded with '"' (double quote).

If you specify 'nil', milter-manager creates".milter-manager" directory under effective user's homedirectory and use it for custom configuration directory.

Example:

<pre>manager.custom_configuration_directory = "/tmp/milter-manager/"</pre>

Default:

<pre>manager.custom_configuration_directory = nil</pre></dd>
<dt>manager.fallback_status</dt>
<dd>Since 1.6.3.

Specifies a status that is replied to the SMTP server onerror in milter manager. Here is a list of error cases inmilter manager:

* No child milter.
* Failed to open a temporary file for mail body.
* and so on...

Here are available values:

* "accept": Accepts a mail. It's the default.
* "temporary-failure": Rejects a mail temporary.
* "reject": Rejects a mail.
* "discard": Discards a mail.

Example:

<pre>manager.fallback_status = "reject"</pre>

Default:

<pre>manager.fallback_status = "accept"</pre></dd>
<dt>manager.fallback_status_at_disconnect</dt>
<dd>Since 1.6.3.

Specifies a status that is replied to the SMTP server whenmilter manager detects the SMTP client disconnects theSMTP server connection. This item doesn't used by defaultbecause disconnect check is disabled by default. Youcan enable disconnect check bymanager.use_netstat_connection_checker.

Here are available values:

* "accept": Accepts a mail.
* "temporary-failure": Rejects a mail temporary. It's the default.
* "reject": Rejects a mail.
* "discard": Discards a mail.

Example:

<pre>manager.fallback_status_at_disconnect = "discard"</pre>

Default:

<pre>manager.fallback_status_at_disconnect = "temporary-failure"</pre></dd>
<dt>manager.event_loop_backend</dt>
<dd><em>Normally, this item doesn't need to be used.</em>

Since 1.6.3.

Specifies a backend for event loop. For non-largemail system which processes 100 or less mails per second,you don't need to change it. For large mail system whichprocesses 100 or more mails per seconds, you need tochange it to "libev".

NOTE: For the sake of implemenation, glib backend uses callback.Note that glib's callback registration upper limit makesthe limitation of the number of communications.This limitations exist against glib backend only.

Here are availble values:

* "glib": Uses GLib's event loop that uses poll(2) as       I/O multiplexer. It's the default.
* "libev": Uses libev that uses epoll, kqueue or event       ports as I/O multiplexer.

Example:

<pre>manager.event_loop_backend = "libev"</pre>

Default:

<pre>manager.event_loop_backend = "glib"</pre></dd>
<dt>manager.n_workers</dt>
<dd><em>Normally, this item doesn't need to be used.</em>

Since 1.6.3.

Specifies the number of processes which process mails. For non-largemail system which processes 100 or less mails per second, and mailsystem which doesn't use very heavy milter,you don't need to change it. For large mail system whichprocesses 100 or more mails per seconds using very heavy milters,you need to increase it.

Availble value is between 0 and 1000.If it is 0, no worker processes will be used.

Example:

<pre>manager.n_workers = 10</pre>

Default:

<pre>manager.n_workers = 0 # no worker processes.</pre></dd>
<dt>manager.packet_buffer_size</dt>
<dd><em>Normally, this item doesn't need to be used.</em>

Since 1.6.3.

Specifies buffer size to buffer send packets onend-of-message. Packets aren't sent until amount ofbuffered packets is greater than buffer size. 0 meansbuffering is disabled.

It may improve performance when many message modificationoperations, add_header, delete_recipient and so on, arerequested on end-of-message. Normary, this configurationdoesn't improve performance.

Example:

<pre>manager.packet_buffer_size = 4096 # Doesn't send packets
                                  # until amount of
                                  # buffered packets >= 4KB.</pre>

Default:

<pre>manager.packet_buffer_size = 0 # Disables buffering.</pre></dd>
<dt>manager.chunk_size</dt>
<dd><em>Normally, this item doesn't need to be used.</em>

Since 1.8.0.

Specifies chunk size on body data for 2..n child milters.The maximum size is 65535 bytes and it is the default. Ifchunk size is decreased, communication overhead isincrased. You should use it only if you want to decreaseeach data size.

Example:

<pre>manager.chunk_size = 4096 # Sends body data as 4KB chunks.</pre>

Default:

<pre>manager.chunk_size = 65535 # Sends body data as 64KB chunks.</pre></dd>
<dt>manager.max_pending_finished_sessions</dt>
<dd><em>Normally, this item doesn't need to be used.</em>

Since 1.8.6.

Milter manager delays processings that don't effect to throughputuntil idle time. For example, termination processing for finishedmilter session is one of those delayed processings.

Milter manager does termination processing for finished miltersessions by setting this item. Normally, there is idle time evenwhen milter manager processes multiple milter sessions concurrently.If the number of processing milter sssions is too large, there willbe no idle time. So termination processins are not done. Iftermination processings are not done for a long time, the number ofopenable file descripters may be lacked. Because used socket isclosed in termination processing.

Normally, you should avoid the no idle time situation because it isoverload. It is better that increasing the number of workers bymanager.n_workers configuration.

Milter manager does termination processing even when there is noidle time by setting one or larger number. Termination processingis done when specified the number of finished sessions. This itemdoesn't disable termination processing on idle time. So this itemdoesn't effect to throughput on normal time. This item effectson only no idle time.

The default value is 0. It distables termination processing on noidle time feature.

Example:

<pre># Do termination processing after each session is finished
manager.max_pending_finished_sessions = 1</pre>

Default:

<pre># Do termination processing when no other processings aren't remining
manager.max_pending_finished_sessions = 0</pre></dd>
<dt>manager.use_netstat_connection_checker</dt>
<dd>Since 1.5.0.

Checks SMTP client and SMTP server are still connected byparsing <kbd>netstat</kbd> command result.

This feature is useful for aborting SMTP session whenSMTP client disconnects SMTP session. e.g. Using[taRgrey](http://k2net.hakuba.jp/targrey/index.en.html)with milter (milter-greylist). This feature resolves aproblem of taRgrey that SMTP server process isgrown. SMTP server process growth means memory usagegrowth. i.e. This feature reduces memory usage caused bytaRgrey.

SMTP session is checked in 5 seconds. The interval timecan be changed but it's not needed normally.

Example:

<pre>manager.use_netstat_connection_checker    # check in 5 seconds.
manager.use_netstat_connection_checker(1) # check in 1 seconds.</pre>

Default:

<pre>Don't check.</pre></dd>
<dt>manager.connection_check_interval</dt>
<dd><em>Normally, this item doesn't need to be used directly.</em>

Since 1.5.0.

Specifies an interval in seconds to check whether aconnection between SMTP client and SMTP server is stillconnected.

0 means 'no check'.

manager.define_connection_checkerdefines how to check whether a connection is stillconnected.

Example:

<pre>manager.connection_check_interval = 5 # Check in 5 seconds.</pre>

Default:

<pre>manager.connection_check_interval = 0</pre></dd>
<dt>manager.define_connection_checker(name) {|context| ... # -&gt; true/false}</dt>
<dd><em>Normally, this item doesn't need to be used directly.</em>

Since 1.5.0.

Checks a SMTP client is still connected to a SMTPserver eachmanager.connection_check_intervalseconds. If given block returns true value, it means thatthe connection is still alive, otherwise it means thatthe connection had been closed.

<dl>
<dt>name</dt>
<dd>The name of the check process.</dd>
<dt>context</dt>
<dd>The object passed to the block that knows the currentsituation. It has the following information:</dd>
<dt>context.smtp_client_address</dt>
<dd>Is the IP address of the check target SMTP client.It is the same object assocket_address.</dd>
<dt>context.smtp_server_address</dt>
<dd>Is the IP address of the accepted SMTP server socket.It is the same object assocket_address.</dd></dl>

Example:

<pre># It assumes that a connection from non local network
# is always closed.
manager.define_connection_checker("netstat-check") do |context|
  context.smtp_client_address.local?
end</pre></dd>
<dt>manager.report_memory_statistics</dt>
<dd>Since 1.5.0.

Logs memory usage each maintenance process.

Here is the output format but it may be changed in thefeature:

<pre>Mar 28 15:16:58 mail milter-manager[19026]: [statistics] [maintain][memory] (28048KB) total:6979 Proc:44 GLib::Object:18</pre>

Example:

<pre>manager.report_memory_statistics</pre></dd>
<dt>manager.maintained {...}</dt>
<dd><em>Normally, this item doesn't need to be used directly.</em>

Since 1.5.0.

Executes a custom process each maintenance process.

Here is an example that logs a message each maintenanceprocess.

Example:

<pre>manager.maintained do
  Milter::Logger.info("maintained!")
end</pre></dd>
<dt>manager.event_loop_created {|loop| ...}</dt>
<dd><em>Normally, this item doesn't need to be used directly.</em>

Since 1.6.8.

Executes a custom process on an event loop is created. Anevent loop is created only on initialization.

Here is an example that registers a callback that logs amessage at intervals of 1 second.

Example:

<pre>manager.event_loop_created do |loop|
  loop.add_timeout(1) do
    Milter::Logger.info("timeout!")
    true
  end
end</pre></dd></dl>

## Controller {#controller}

<dl>
<dt>controller.connection_spec</dt>
<dd>Specifies a socket that milter-manager accepts connectionfor controlling milter-manager.

Format is same as manager.connection_spec.

Example:

<pre>controller.connection_spec = "inet:10026@localhost"</pre>

Default:

<pre>controller.connection_spec = nil</pre></dd>
<dt>controller.unix_socket_mode</dt>
<dd>Specifies permission of UNIX domain socket forcontrolling milter-manager. This is used only when UNIXdomain socket is specified for controller.connection_spec.

Don't forget to prepend '0' to specify permission asoctal notation.

Example:

<pre>controller.unix_socket_mode = 0600</pre>

Default:

<pre>controller.unix_socket_mode = 0660</pre></dd>
<dt>controller.remove_unix_socket_on_create</dt>
<dd>Specifies whether remove existing UNIX domain socketfor controlling milter-manager before creating new UNIXdomain socket or not. This is used only when UNIX domainsocket is specified for controller.connection_spec.

True if remove, false otherwise.

Example:

<pre>controller.remove_unix_socket_on_create = false</pre>

Default:

<pre>controller.remove_unix_socket_on_create = true</pre></dd>
<dt>controller.remove_unix_socket_on_close</dt>
<dd>Specifies whether remove used UNIX domain socket forcontrolling milter-manager after shutting down. This isused only when UNIX domain socket is specified forcontroller.connection_spec.

True if remove, false otherwise.

Example:

<pre>controller.remove_unix_socket_on_close = false</pre>

Default:

<pre>controller.remove_unix_socket_on_close = true</pre></dd></dl>

## Child milter {#child-milter}

This section describes about configuration items relatedwith child milter.

### Define child milter

Child milter is registered as the following syntax:

<pre>define_milter("NAME") do |milter|
  milter.XXX = ...
  milter.YYY = ...
  milter.ZZZ = ...
end</pre>

For example, to register a milter that accepts connection at'inet:10026@localhost' as 'test-milter':

<pre>define_milter("test-milter") do |milter|
  milter.connection_spec = "inet:10026@localhost"
end</pre>

The following items can be used in 'define_milter do ... end'.

Required item is just only milter.connection_spec.

<dl>
<dt>milter.connection_spec</dt>
<dd>Specifies socket that the child milter accepts.This is <em>required item</em>.

Format is same as manager.connection_spec.

Example:

<pre>milter.connection_spec = "inet:10026@localhost"</pre>

Default:

<pre>milter.connection_spec = nil</pre></dd>
<dt>milter.description</dt>
<dd>Specifies description of the child milter.

Description is specified like "test milter". Description issurrounded with '"' (double quote).

Example:

<pre>milter.description = "test milter"</pre>

Default:

<pre>milter.description = nil</pre></dd>
<dt>milter.enabled</dt>
<dd>Whether use the child milter or not.

True if use, false otherwise.

Example:

<pre>milter.enabled = false</pre>

Default:

<pre>milter.enabled = true</pre></dd>
<dt>milter.fallback_status</dt>
<dd>Specifies a status that is used the child milter causesan error.

Here are available values:

* "accept": Accepts a mail. It's the default.
* "temporary-failure": Rejects a mail temporary.
* "reject": Rejects a mail.
* "discard": Discards a mail.

Example:

<pre>milter.fallback_status = "temporary-failure"</pre>

Default:

<pre>milter.fallback_status = "accept"</pre></dd>
<dt>milter.evaluation_mode</dt>
<dd>Since 1.3.1.

Whether turn on evaluation mode or not. The child milterdoesn't return its result on evaluation mode. It meansthe child milter doesn't affect the existing mail system.

Graphs are still generated on evaluation mode becausestatistics are logged.

True if evaluation mode is turned on, false otherwise.

On false (default) case, message handling in miltersession is aborted when a child milter returns "reject"because milter manager returns "reject" to MTA. On truecase, message handling in milter session is continuedwhen a child milter returns "reject" because miltermanager doesn't return "reject" to MTA. You can use acondition that "a child milter returns 'reject'" inapplicable conditions.

Example:

<pre>milter.evaluation_mode = true</pre>

Default:

<pre>milter.evaluation_mode = false</pre></dd>
<dt>milter.applicable_conditions</dt>
<dd>Specifies applicable conditions for the child milter. Thechild milter is stopped if any condition isn't satisfied.

The following command shows available applicableconditions:

<pre>% /usr/local/sbin/milter-manager --show-config | grep define_applicable_condition
define_applicable_condition("S25R") do |condition|
define_applicable_condition("Remote Network") do |condition|</pre>

In the above case, "S25R" and "Remote Network" areavailable.

Some applicable conditions are available in defaultconfiguration. You can also define your originalapplicable condition. SeeDefine applicable conditionabout how to define applicable condition. But definingapplicable condition requires Ruby's knowledge.

To specify multiple applicable conditions, you can use",":

<pre>milter.applicable_conditions = ["S25R", "Remote Network"]</pre>

Example:

<pre>milter.applicable_conditions = ["S25R"]</pre>

Default:

<pre>milter.applicable_conditions = []</pre></dd>
<dt>milter.add_applicable_condition(name)</dt>
<dd>Adds an applicable condition to the child milter. Seemilter.applicable_conditions about 'applicable condition'.

Example:

<pre>milter.add_applicable_condition("S25R")</pre></dd>
<dt>milter.command</dt>
<dd>Specifies command that run the child milter.The child milter is ran by the command automatically whenconnecting milter.connection_spec is failed. It isenabled only if security.privilege_mode is true andmilter-manager is ran.

Command may be run script that is placed in/etc/init.d/ or /usr/local/etc/rc.d/.

Command is specified like"/etc/init.d/milter-greylist". Command should be surroundedwith '"' (double quote). If you don't want to runautomatically, use nil.

Example:

<pre>milter.command = "/etc/init.d/milter-greylist"</pre>

Default:

<pre>milter.command = nil</pre></dd>
<dt>milter.command_options</dt>
<dd>Specifies options to be passed to milter.command.

Options are specified like "start". Options should besurrounded with '"' (double quote).  If some options arespecified, use "--option1 --option2" or ["--option1","--option2"].

Example:

<pre>milter.command_options = "start"
milter.command_options = ["--option1", "--option2"]</pre>

Default:

<pre>milter.command_options = nil</pre></dd>
<dt>milter.user_name</dt>
<dd>Specifies user name to run milter.command.

User name is specified like "nobody". User name should besurrounded with '"' (double quote). If you want to runmilter.command as root, use nil.

Example:

<pre>milter.user_name = "nobody"</pre>

Default:

<pre>milter.user_name = nil</pre></dd>
<dt>milter.connection_timeout</dt>
<dd>Specifies timeout in seconds for trying to connect tochild milter.

Example:

<pre>milter.connection_timeout = 60</pre>

Default:

<pre>milter.connection_timeout = 297.0</pre></dd>
<dt>milter.writing_timeout</dt>
<dd>Specifies timeout in seconds for trying to write tochild milter.

Example:

<pre>milter.writing_timeout = 15</pre>

Default:

<pre>milter.writing_timeout = 7.0</pre></dd>
<dt>milter.reading_timeout</dt>
<dd>Specifies timeout in seconds for trying to read fromchild milter.

Example:

<pre>milter.reading_timeout = 15</pre>

Default:

<pre>milter.reading_timeout = 7.0</pre></dd>
<dt>milter.end_of_message_timeout</dt>
<dd>Specifies timeout in seconds for trying to wait responseof xxfi_eom() from child milter.

Example:

<pre>milter.end_of_message_timeout = 60</pre>

Default:

<pre>milter.end_of_message_timeout = 297.0</pre></dd>
<dt>milter.name</dt>
<dd>Since 1.8.1.

Returns child milter's name specified by define_milter.</dd></dl>

### Operate child milter

There is convenient features to operate defined childmilters. But the features require knowledge about Ruby abit.

defined_milters returns a list of define child milter names.

<pre>define_milter("milter1") do |milter|
  ...
end

define_milter("milter2") do |milter|
  ...
end

defined_milters # => ["milter1", "milter2"]</pre>

It's easy that changing configuration of all child milter bythis feature.

Here is an example that disabling all child milters:

<pre>defined_milters.each do |name|
  define_milter(name) do |milter|
    milter.enabled = false
  end
end</pre>

Here is an example that removing all child milters:

<pre>defined_milters.each do |name|
  remove_milter(name)
end</pre>

Removing differs from disabling. You need to redefine amilter again when you want to use a removed milter again.

Here is an example that adding "S25R" applicable conditionto all child milters.

<pre>defined_milters.each do |name|
  define_milter(name) do |milter|
    milter.add_applicable_condition("S25R")
  end
end</pre>

<dl>
<dt>defined_milters</dt>
<dd>Returns a list of defined child milter names. Returnedvalue is an array of string.

Example:

<pre>defined_milters # => ["milter1", "milter2"]</pre></dd>
<dt>remove_milter(name)</dt>
<dd>Removes a child milter that is named as 'name'. Youshould use milter.enabled when youmay reuse the child milter.

Example:

<pre># Removes a milter that is defined as "milter1".
remove_milter("milter1")</pre></dd></dl>

## Built-in applicable conditions {#built-in-applicable-conditions}

Here are descriptions about built-in applicable conditions.

### S25R

This applicable condition applies a child milter to onlynormal PC like SMTP client. A child milter isn't applied toMTA like SMTP client.

Here is an example that uses[Rgrey](http://lists.ee.ethz.ch/postgrey/msg01214.html)technique. (NOTE: milter-greylist should have "racl greylistdefault" configuration.)

Example:

<pre>define_milter("milter-greylist") do |milter|
  milter.add_applicable_condition("S25R")
end</pre>

See [S25R](http://www.gabacho-net.jp/en/anti-spam/)how to determine a SMTP client is MTA or normal PC.

S25R rules will also match non normal PC host name. To avoidthis false positive, you can create whitelist. google.comdomain and obsmtp.com domain are in whitelist by default.

You can also create blacklist for normal PC host name thatisn't matched to S25R rules.

You can customize S25R applicable condition by the followingconfigurations:

<dl>
<dt>s25r.add_whitelist(matcher)</dt>
<dd>Since 1.5.2.

S25R applicable condition treats a host name that<var>matcher</var> matches the host name as MTA host and addsthe host name to whitelist. If ahost name is listed in whitelist, child milter isn'tapplied.

<var>matcher</var> is a regular expression or a host name asstring.

For example, the following configuration adds google.comdomain to whitelist:

<pre>s25r.add_whitelist(/\.google\.com\z/)</pre>

The following configuration adds mx.example.com host towhitelist:

<pre>s25r.add_whitelist("mx.example.com")</pre>

[For power user] You can specify complex condition byblock. S25R applicable condition passes a host name tothe block. For example, the following configuration adds.jp top level domain while 8:00 a.m. to 7:59 p.m. towhitelist:

<pre>s25r.add_whitelist do |host|
  (8..19).include?(Time.now.hour) and /\.jp\z/ === host
end</pre></dd>
<dt>s25r.add_blacklist(matcher)</dt>
<dd>Since 1.5.2.

S25R applicable condition treats a host name that<var>matcher</var> matches the host name as normal PC host andadds the host to blacklist. If a host name is listed inblacklist, child milter is applied.

NOTE: If a host is listed both whitelist and blacklist,S25R applicable condition give preference to whitelistover blacklist. That is, child milter isn't applied inthe case.

<var>matcher</var> is a regular expression or a host name asstring.

For example, the following configuration addsevil.example.com domain to blacklist:

<pre>s25r.add_blacklist(/\.evil\.example\.com\z/)</pre>

The following configuration adds black.example.com host toblacklist:

<pre>s25r.add_blacklist("black.example.com")</pre>

[For power user] You can specify complex condition byblock. S25R applicable condition passes a host name tothe block. For example, the following configuration adds.jp top level domain while 8:00 p.m. to 7:59 a.m. toblacklist:

<pre>s25r.add_blacklist do |host|
  !(8..19).include?(Time.now.hour) and /\.jp\z/ === host
end</pre></dd>
<dt>s25r.check_only_ipv4=(boolean)</dt>
<dd>Since 1.6.6.

If <code>true</code> is specified, S25R check is enabled onlyfor IPv4 connection. If <code>false</code> is specified, S25Rcheck is also enabled for IPv6 connection.

Example:

<pre>s25r.check_only_ipv4 = false # enabled for non IPv4 connection</pre>

Default:

<pre>S25R check is enabled only for IPv4.</pre></dd></dl>

### Remote Network

This applicable condition applies a child milter to onlySMTP client that is connected from remote network.

Here is an example that mails from local network are skippedspam-check to avoid false detection:

Example:

<pre>define_milter("spamass-milter") do |milter|
  milter.add_applicable_condition("Remote Network")
end</pre>

Local network means that not private IPaddress. e.g. 192.168.0.0/24. You can customize local networkby the following configurations.

<dl>
<dt>remote_network.add_local_address(address)</dt>
<dd>Since 1.5.0.

Adds the specified IPv4/IPv6 address or IPv4/IPv6 networkto local network. Child milter isn't applied to SMTPclients connected from local network.

Example:

<pre># Don't apply child milters connections from 160.29.167.10.
remote_network.add_local_address("160.29.167.10")
# Don't apply child milters connections from
# 160.29.167.0/24 network.
remote_network.add_local_address("160.29.167.0/24")
# Don't apply child milters connections from
# 2001:2f8:c2:201::fff0.
remote_network.add_local_address("2001:2f8:c2:201::fff0")
# Don't apply child milters connections from
# 2001:2f8:c2:201::/64 network.
remote_network.add_local_address("2001:2f8:c2:201::/64")</pre></dd>
<dt>remote_network.add_remote_address(address)</dt>
<dd>Since 1.5.0.

Adds the specified IPv4/IPv6 address or IPv4/IPv6 networkto remote network. Child milter is applied to SMTPclients connected from remote network.

Example:

<pre># Apply child milters connections from 160.29.167.10.
remote_network.add_remote_address("160.29.167.10")
# Apply child milters connections from
# 160.29.167.0/24 network.
remote_network.add_remote_address("160.29.167.0/24")
# Apply child milters connections from
# 2001:2f8:c2:201::fff0.
remote_network.add_remote_address("2001:2f8:c2:201::fff0")
# Apply child milters connections from
# 2001:2f8:c2:201::/64 network.
remote_network.add_remote_address("2001:2f8:c2:201::/64")</pre></dd></dl>

### Authentication {#authentication}

This applicable condition applies a child milter toauthenticated SMTP client by SMTP Auth. MTA should sendauthentication related macros to a milter. Sendmail isn'tneeded additional configuration. Postfix needs the followingadditional configuration:

main.cf:

<pre>milter_mail_macros = {auth_author} {auth_type} {auth_authen}</pre>

Here is an example that authenticated SMTP client's mail,inner to outer mail, is Bcc-ed to be audited:

Example:

<pre>define_milter("milter-bcc") do |milter|
  milter.add_applicable_condition("Authenticated")
end</pre>

### Unauthentication {#unauthentication}

This applicable condition applies a child miter tonon-authenticated SMTP client by SMTP Auth. MTA should sendauthentication related macros to a milter. See alsoAuthentication.

Here is an example that only non-authentication SMTP clientis applied spam-check to avoid false detection.

Example:

<pre>define_milter("spamass-milter") do |milter|
  milter.add_applicable_condition("Unauthenticated")
end</pre>

### Sendmail Compatible

This applicable condition is a bit strange. This alwaysapplies a child milter. This substitute different macrosbetween Sendmail and Postfix. A milter that depends onSendmail specific macros can be worked with this applicablecondition and Postfix.

This applicable condition will be needless in the nearfeature because milters are fixing to be work with bothSendmail and Postfix. It's good things.

You can use this applicable condition with Postfix, itdoesn't have adverse affect for Postfix.

Here is an example that you use milter-greylist built forSendmail with Postfix.

Example:

<pre>define_milter("milter-greylist") do |milter|
  milter.add_applicable_condition("Sendmail Compatible")
end</pre>

### stress

Since 1.5.0.

Those applicable conditions changes process depends onstress dynamically. Stress is determine by number ofconcurrent connections.

<dl>
<dt>stress.threshold_n_connections</dt>
<dd>Since 1.5.0.

Returns number of concurrent connections to determinestressed.

With Postfix, number of max smtpd processes are detectedautomatically and 3/4 of it is set.

With Sendmail, it's not detected automatically. You needto set it withstress.threshold_n_connections=by hand.

Example:

<pre># Postfix's default. (depends on our environment)
stress.threshold_n_connections # => 75</pre></dd>
<dt>stress.threshold_n_connections=(n)</dt>
<dd>Since 1.5.0.

Sets number of connections to determine stressed.

0 means that always non-stressed.

Example:

<pre># Number of concurrent connections is higher or equal
# 75 means stressed.
stress.threshold_n_connections = 75</pre></dd></dl>

#### No Stress {#no-stress}

Since 1.5.0.

This applies a child milter only when non-stressed.

Here is an example that spamass-milter isn't applied whenstressed:

Example:

<pre>define_milter("spamass-milter") do |milter|
  milter.add_applicable_condition("No Stress")
end</pre>

#### Stress Notify {#stress-notify}

Since 1.5.0.

This notifies stressed to a child milter by "{stress}=yes"macro. This just notifies. It means that a child milter isalways applied.

Here is an example that milter-greylist is notified stressedby macro.

Example:

<pre>define_milter("milter-greylist") do |milter|
  milter.add_applicable_condition("Stress Notify")
end</pre>

Here is an example configuration for milter-greylist to usetarpitting on non-stress and greylisting on stress. Thisconfiguration requires milter-greylist 4.3.4 or later.

greylist.conf:

<pre>sm_macro "no_stress" "{stress}" unset
racl whitelist sm_macro "no_stress" tarpit 125s
racl greylist default</pre>

### Trust {#trust}

Since 1.6.0.

This sets "trusted_XXX=yes" macros for trusted session. Hereis a list of macros:

<dl>
<dt>trusted_domain</dt>
<dd>This is set to "yes" when envelope-from domain is trusted.</dd></dl>

Here is an example that you receive a SPF passed mail butapply greylist SPF not-passed mail for trusted domains:

milter-manager.local.conf:

<pre>define_milter("milter-greylist") do |milter|
  milter.add_applicable_condition("Trust")
end</pre>

greylist.conf:

<pre>sm_macro "trusted_domain" "{trusted_domain}" "yes"
racl whitelist sm_macro "trusted_domain" spf pass
racl greylist sm_macro "trusted_domain" not spf pass</pre>

You can customize how to trust a session by the followingconfigurations:

<dl>
<dt>trust.add_envelope_from_domain(domain)</dt>
<dd>Since 1.6.0.

This adds a trusted envelope-from domain.

Here is a list of trusted domain by default:

* gmail.com
* hotmail.com
* msn.com
* yahoo.co.jp
* softbank.ne.jp
* clear-code.com

Example:

<pre>trust.add_envelope_from_domain("example.com")</pre></dd>
<dt>trust.clear</dt>
<dd>Since 1.8.0.

This clears all registered trusted envelope-from domain list.

Example:

<pre>trust.clear</pre></dd>
<dt>trust.load_envelope_from_domains(path)</dt>
<dd>Since 1.8.0.

This loads trusted envelope-from domain list from<var>path</var>. Here is <var>path</var> format:

<pre># This is comment line. This line is ignored.
gmail.com
# The above line means 'gmail.com is trusted domain'.
/\.example\.com/
# The above line means 'all sub domains of example.com are trusted'.

# The above line consists of spaces. The space only line is ignored.</pre>

Example:

<pre>trust.load_envelope_from_domains("/etc/milter-manager/trusted-domains.list")
# It loads trusted envelope-from domain list from
# /etc/milter-manager/trusted-domains.list.</pre></dd></dl>

### Restrict Accounts

This applies milters if recipients include envelope-recipient ismatched the condition.

<dl>
<dt>restrict_accounts_by_list(*accounts, condition_name: "Restrict Accounts by List: #{accounts.inspect}", milters: defined_milters)</dt>
<dd>Specify <var>accounts</var> that you want to apply milters.Last arguments can be a Hash that includes condition name and milters.Call <var>restrict_accounts_generic</var> internally.

Example:

<pre>restrict_accounts_by_list("bob@example.com", /@example\.co\.jp\z/, condition_name: "Restrict Accounts")</pre></dd>
<dt>restrict_accounts_generic(options, &amp;restricted_account_p)</dt>
<dd><var>options</var> is a Hash that includes <var>condition_name</var> and <var>milters</var>.Block parameters are <var>context</var> and <var>recipient</var>.</dd></dl>

## Define applicable condition {#applicable-condition}

You need knowledge about Ruby from this section. Some usefulapplicable conditions are provided by default. You can definenew applicable conditions if you need more conditions. You candecide which child milter is applied or not dynamically bydefining our applicable conditions.

You define an applicable condition with the followingsyntax. You need knowledge about Ruby for defining applicablecondition.

<pre>define_applicable_condition("NAME") do |condition|
  condition.description = ...
  condition.define_connect_stopper do |...|
    ...
  end
  ...
end</pre>

For example, here is an applicable condition to implementS25R:

<pre>define_applicable_condition("S25R") do |condition|
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
end</pre>

'host' is "[IP ADDRESS]" not "unknown" when name resolutionis failed. So, "unknown" is needless. It's sufficient thatyou just use /\A\[.+\]\z/. But it is included just in case. :-)

Here are configurable items in 'define_applicable_conditiondo ... end'.

There is no required item.

<dl>
<dt>condition.description</dt>
<dd>Specifies description of the applicable condition.

Description is specified like "test condition". Description issurrounded with '"' (double quote).

Example:

<pre>condition.description = "test condition"</pre>

Default:

<pre>condition.description = nil</pre></dd>
<dt>condition.define_connect_stopper {|context, host, socket_address| ...}</dt>
<dd>Decides whether the child milter is applied or not withhost name and IP address of connected SMTP client. Theavailable information is same as milter's xxfi_connect.

It returns true if you stop the child milter, false otherwise.

<dl>
<dt>context</dt>
<dd>The object that has several information at thetime. Details are said later.</dd>
<dt>host</dt>
<dd>The host name (string) of connected SMTP client. It isgot by resolving connected IP address. It may be "[IPADDRESS]" string when name resolution is failed. Forexample, "[1.2.3.4]".</dd>
<dt>[] socket_address</dt>
<dd>The object that describes connected IPaddress. Details are said later.</dd></dl>

Here is an example that you stop the child milter whenSMTP client is connected from resolvable host:

<pre>condition.define_connect_stopper do |context, host, socket_address|
  if /\A\[.+\]\z/ =~ host
    false
  else
    true
  end
end</pre></dd>
<dt>condition.define_helo_stopper {|context, fqdn| ...}</dt>
<dd>Decides whether the child milter is applied or not withFQDN on HELO/EHLO command. The available information issame as milter's xxfi_helo.

It returns true if you stop the child milter, false otherwise.

<dl>
<dt>context</dt>
<dd>The object that has several information at thetime. Details are said later.</dd>
<dt>fqdn</dt>
<dd>The FQDN (string) sent by SMTP client on HELO/EHLO.</dd></dl>

Here is an example that you stop the child milter whenFQDN is "localhost.localdomain".

<pre>condition.define_helo_stopper do |context, helo|
  helo == "localhost.localdomain"
end</pre></dd>
<dt>define_envelope_from_stopper {|context, from| ...}</dt>
<dd>Decides whether the child milter is applied or not withenvelope from address passed on MAIL FROM command ofSMTP. The available information is same as milter'sxxfi_envfrom.

It returns true if you stop the child milter, false otherwise.

<dl>
<dt>context</dt>
<dd>The object that has several information at thetime. Details are said later.</dd>
<dt>from</dt>
<dd>The envelope from address passed on MAIL FROM command.For example, "&lt;sender@example.com&gt;".</dd></dl>

Here is an example that you stop the child milter whenmails are sent from example.com.

<pre>condition.define_envelope_from_stopper do |context, from|
  if /@example.com>\z/ =~ from
    true
  else
    false
  end
end</pre></dd>
<dt>define_envelope_recipient_stopper {|context, recipient| ...}</dt>
<dd>Decides whether the child milter is applied or not withenvelope recipient address passed on RCPT TO command ofSMTP. The available information is same as milter'sxxfi_envrcpt. This callback is called one or more timeswhen there are multiple recipients.

It returns true if you stop the child milter, false otherwise.

<dl>
<dt>context</dt>
<dd>The object that has several information at thetime. Details are said later.</dd>
<dt>recipient</dt>
<dd>The envelope recipient address passed on RCPT TO command.For example, "&lt;receiver@example.com&gt;".</dd></dl>

Here is an example that you stop the child milter whenmails are sent to ml.example.com.

<pre>condition.define_envelope_recipient_stopper do |context, recipient|
  if /@ml.example.com>\z/ =~ recipient
    true
  else
    false
  end
end</pre></dd>
<dt>condition.define_data_stopper {|context| ...}</dt>
<dd>Decides whether the child milter is applied or not onDATA. The available information is same as milter'sxxfi_data.

It returns true if you stop the child milter, false otherwise.

<dl>
<dt>context</dt>
<dd>The object that has several information at thetime. Details are said later.</dd></dl>

Here is an example that you stop the child milter on DATA.milter can only add/delete/modify header and/or bodyafter whole message is processed. If you stop the childmilter on DATA, you ensure that milter don'tadd/delete/modify header and/or body. You can confirmthe child milter's work if the child milter logs itsprocessed result.

<pre>condition.define_data_stopper do |context|
  true
end</pre></dd>
<dt>define_header_stopper {|context, name, value| ...}</dt>
<dd>Decides whether the child milter is applied or not withheader information. The available information is same asmilter's xxfi_header. This callback is called for eachheader.

It returns true if you stop the child milter, false otherwise.

<dl>
<dt>context</dt>
<dd>The object that has several information at thetime. Details are said later.</dd>
<dt>name</dt>
<dd>The header name. For example, "From".</dd>
<dt>value</dt>
<dd>The header value. For example, "sender@example.com".</dd></dl>

Here is an example that you stop the child milter whenmails have a header that name is "X-Spam-Flag" and valueis "YES".

<pre>condition.define_header_stopper do |context, name, value|
  if ["X-Spam-Flag", "YES"] == [name, value]
    true
  else
    false
  end
end</pre></dd>
<dt>condition.define_end_of_header_stopper {|context| ...}</dt>
<dd>Decides whether the child milter is applied or not afterall headers are processed. The available information issame as milter's xxfi_eoh.

It returns true if you stop the child milter, false otherwise.

<dl>
<dt>context</dt>
<dd>The object that has several information at thetime. Details are said later.</dd></dl>

Here is an example that you stop the child milter after allheaders are processed.

<pre>condition.define_end_of_header_stopper do |context|
  true
end</pre></dd>
<dt>condition.define_body_stopper {|context, chunk| ...}</dt>
<dd>Decides whether the child milter is applied or not witha body chunk. The available information is same asmilter's xxfi_body. This callback may be called multipletimes for a large body mail.

It returns true if you stop the child milter, false otherwise.

<dl>
<dt>context</dt>
<dd>The object that has several information at thetime. Details are said later.</dd></dl>

Here is an example that you stop the child milter after allheaders are processed.

<dl>
<dt>chunk</dt>
<dd>A chunk of body. A large body is not processed at atime. It is processed as small chunks. The maximumchunk size is 65535 byte.</dd></dl>

Here is an example that you stop the child milter whenchunk contains PGP signature.

<pre>condition.define_body_stopper do |context, chunk|
  if /^-----BEGIN PGP SIGNATURE-----$/ =~ chunk
    true
  else
    false
  end
end</pre></dd>
<dt>condition.define_end_of_message_stopper {|context| ...}</dt>
<dd>Decides whether the child milter is applied or not aftera mail is processed. The available information issame as  milter's xxfi_eom.

It returns true if you stop the child milter, false otherwise.

<dl>
<dt>context</dt>
<dd>The object that has several information at thetime. Details are said later.</dd></dl>

Here is an example that you stop the child milter aftera mail is processed.

<pre>condition.define_end_of_message_stopper do |context|
  true
end</pre></dd></dl>

### context

The object that has several information when you decidewhether a child milter is applied or not. (The class ofcontext is Milter::Manager::ChildContext.)

It has the following information.

<dl>
<dt>context.name</dt>
<dd>Returns the name of child milter. It's name used whendefine_milter.

Example:

<pre>context.name # -> "clamav-milter"</pre></dd>
<dt>context[name]</dt>
<dd>Returns value of available macro in the child milter Inlibmilter API, you need to surround macro name that hastwo or more length with "{}" but it isn'trequired. context[name] works well with/without "{}".

Example:

<pre>context["j"] # -> "mail.example.com"
context["rcpt_address"] # -> "receiver@example.com"
context["{rcpt_address}"] # -> "receiver@example.com"</pre></dd>
<dt>context.reject?</dt>
<dd>Returns true if the child milter returns 'reject'.The child milter must be enabledmilter.evaluation_mode.

Passed context should be always processing. So,context.reject? never return true. It's usuful when youuse the other child milter's result. The other child canbe retrieved by context.children[].

Example:

<pre>context.reject? # -> false
context.children["milter-greylist"].reject? # -> true or false</pre></dd>
<dt>context.temporary_failure?</dt>
<dd>Returns true if the child milter returns 'temporary failure'.The child milter must be enabledmilter.evaluation_mode.

Passed context should be always processing. So,context.temporay_failure? never return true. It's usufulwhen you use the other child milter's result. The otherchild can be retrieved by context.children[].

Example:

<pre>context.temporary_failure? # -> false
context.children["milter-greylist"].temporary_failure? # -> true or false</pre></dd>
<dt>context.accept?</dt>
<dd>Returns true if the child milter returns 'accept'.

Passed context should be always processing. So,context.accept? never return true. It's usufulwhen you use the other child milter's result. The otherchild can be retrieved by context.children[].

Example:

<pre>context.accept? # -> false
context.children["milter-greylist"].accept? # -> true or false</pre></dd>
<dt>context.discard?</dt>
<dd>Returns true if the child milter returns 'discard'.The child milter must be enabledmilter.evaluation_mode.

Passed context should be always processing. So,context.discard? never return true. It's usufulwhen you use the other child milter's result. The otherchild can be retrieved by context.children[].

Example:

<pre>context.discard? # -> false
context.children["milter-greylist"].discard? # -> true or false</pre></dd>
<dt>context.quitted?</dt>
<dd>Returns true if the child milter was quitted.

Passed context should be always processing. So,context.quitted? never return true. It's usufulwhen you use the other child milter's result. The otherchild can be retrieved by context.children[].

Example:

<pre>context.quitted? # -> false
context.children["milter-greylist"].quitted? # -> true or false</pre></dd>
<dt>context.children[name]</dt>
<dd>Returnes the other child milter's context.

The name to refere the other child milter is a name thatis used for define_milter. (i.e. the name returned bycontext.name)

It returns nil if you refer with nonexistent name.

Example:

<pre>context.children["milter-greylist"] # -> milter-greylist's context
context.children["nonexistent"]     # -> nil
context.children[context.name]      # -> my context</pre></dd>
<dt>context.postfix?</dt>
<dd>Returns true when MTA is Postfix. It is decided by "v"macro's value. If the value includes "Postfix", MTA willbe Postfix.

Returns true when MTA is Postfix, false otherwise.

Example:

<pre>context["v"]     # -> "Postfix 2.5.5"
context.postfix? # -> true

context["v"]     # -> "2.5.5"
context.postfix? # -> false

context["v"]     # -> nil
context.postfix? # -> false</pre></dd>
<dt>context.authenticated?</dt>
<dd>Returns true when sender is authenticated. It is decided by"auto_type" macro or "auth_authen" macro isavailable. They are available since MAIL FROM. So, italways returns false before MAIL FROM. You don't forget toadd the following configuration to main.cf if you areusing Postfix.

<pre>milter_mail_macros = {auth_author} {auth_type} {auth_authen}</pre>

It returns true when sender is authenticated, false otherwise.

Example:

<pre>context["auth_type"]   # -> nil
context["auth_authen"] # -> nil
context.authenticated? # -> false

context["auth_type"]   # -> "CRAM-MD5"
context["auth_authen"] # -> nil
context.authenticated? # -> true

context["auth_type"]   # -> nil
context["auth_authen"] # -> "sender"
context.authenticated? # -> true</pre></dd>
<dt>context.mail_transaction_shelf</dt>
<dd>Child milters can share data while mail transaction.

Since 2.0.5 [Experimental]

Example:

<pre>define_applicable_condition("") do |condition|
  condition.define_envelope_recipient_stopper do |context, recipient|
    if /\Asomeone@example.com\z/ =~ recipient
      context.mail_transaction_shelf["stop-on-data"] = true
    end
    false
  end
  condition.define_data_stopper do |context|
    context.mail_transaction_shelf["stop-on-data"]
  end
end</pre></dd></dl>

### socket_address {#socket-address}

The object that describes socket address. Socket is one ofIPv4 socket, IPv6 socket and UNIX domain socket. Socketaddress is described as corresponding object.

#### Milter::SocketAddress::IPv4

It describes IPv4 socket address. It has the following methods.

<dl>
<dt>address</dt>
<dd>Returns dot-notation IPv4 address.

Example:

<pre>socket_address.address # -> "192.168.1.1"</pre></dd>
<dt>port</dt>
<dd>Returns port number.

Example:

<pre>socket_address.port # -> 12345</pre></dd>
<dt>to_s</dt>
<dd>Returns IPv4 address formated as connection_spec format.

Example:

<pre>socket_address.to_s # -> "inet:12345@[192.168.1.1]"</pre></dd>
<dt>local?</dt>
<dd>Returns true if the address is private network address,false otherwise.

Example:

<pre>socket_address.to_s   # -> "inet:12345@[127.0.0.1]"
socket_address.local? # -> true

socket_address.to_s   # -> "inet:12345@[192.168.1.1]"
socket_address.local? # -> true

socket_address.to_s   # -> "inet:12345@[160.XXX.XXX.XXX]"
socket_address.local? # -> false</pre></dd>
<dt>to_ip_address</dt>
<dd>Returnes corresponding IPAddr object.

Example:

<pre>socket_address.to_s          # -> "inet:12345@[127.0.0.1]"
socket_address.to_ip_address # -> #<IPAddr: IPv4:127.0.0.1/255.255.255.255></pre></dd></dl>

#### Milter::SocketAddress::IPv6

It describes IPv6 socket address. It has the following methods.

<dl>
<dt>address</dt>
<dd>Returns colon-notation IPv6 address.

Example:

<pre>socket_address.address # -> "::1"</pre></dd>
<dt>port</dt>
<dd>Returns port number.

Example:

<pre>socket_address.port # -> 12345</pre></dd>
<dt>to_s</dt>
<dd>Returns IPv6 address formated as connection_spec format.

Example:

<pre>socket_address.to_s # -> "inet6:12345@[::1]"</pre></dd>
<dt>local?</dt>
<dd>Returns true if the address is private network address,false otherwise.

Example:

<pre>socket_address.to_s   # -> "inet6:12345@[::1]"
socket_address.local? # -> true

socket_address.to_s   # -> "inet6:12345@[fe80::XXXX]"
socket_address.local? # -> true

socket_address.to_s   # -> "inet6:12345@[2001::XXXX]"
socket_address.local? # -> false</pre></dd>
<dt>to_ip_address</dt>
<dd>Returnes corresponding IPAddr object.

Example:

<pre>socket_address.to_s          # -> "inet6:12345@[::1]"
socket_address.to_ip_address # -> #<IPAddr: IPv6:0000:0000:0000:0000:0000:0000:0000:0001/ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff></pre></dd></dl>

#### Milter::SocketAddress::Unix

It describes UNIX domain ssocket address. It has the following methods.

<dl>
<dt>path</dt>
<dd>Returns path of the socket.

Example:

<pre>socket_address.path # -> "/tmp/local.sock"</pre></dd>
<dt>to_s</dt>
<dd>Returns UNIX domain socket address formated asconnection_spec format.

Exampel:

<pre>socket_address.to_s # -> "unix:/tmp/local.sock"</pre></dd>
<dt>local?</dt>
<dd>Always returns true.

Example:

<pre>socket_address.local? # -> true</pre></dd>
<dt>to_ip_address</dt>
<dd>Always returnes nil.

Example:

<pre>socket_address.to_s          # -> "unix:/tmp/local.sock"
socket_address.to_ip_address # -> nil</pre></dd></dl>

## Database {#database}

Since 1.6.6.

You need knowledge about Ruby for this section. Miltermanager supports[ActiveRecord](http://api.rubyonrails.org/files/activerecord/README_rdoc.html)as database operation library. You can use many RDB likeMySQL, SQLite3 and so on because ActiveRecord supports them.

You need to install ActiveRecord to use database. Seeoptional install documents for ActiveRecordinstall.

Let's start small example for using 'users' table in MySQL.

Here is connection information for MySQL server:

<dl>
<dt>Database name</dt>
<dd>mail-system</dd>
<dt>IP address of database server</dt>
<dd>192.168.0.1</dd>
<dt>User name</dt>
<dd>milter-manager</dd>
<dt>Password</dt>
<dd>secret</dd></dl>

First, you write the above connection information tomilter-manager.local.conf. In this example,milter-manager.local.conf is placed at/etc/milter-manager/milter-manager.local.conf.

/etc/milter-manager/milter-manager.local.conf:

<pre>database.type = "mysql2"
database.name = "mail-system"
database.host = "192.168.0.1"
database.user = "milter-manager"
database.password = "secret"</pre>

Next, you define ActiveRecord object to operate 'users'table. Definition files are placed at models/directory. models/ directory is placed at the same directoryof milter-manager.local.conf. You create models/user.rb.

/etc/milter-manager/models/user.rb:

<pre>class User < ActiveRecord::Base
end</pre>

Last, you connect to database and operate data inmilter-manager.local.conf:

/etc/milter-manager/milter-manager.local.conf:

<pre>database.setup
database.load_models("models/*.rb")
User.all.each do |user|
  p user.name # => "alice", "bob", ...
end</pre>

Here are completed files:

/etc/milter-manager/milter-manager.local.conf:

<pre># Configure connection information
database.type = "mysql2"
database.name = "mail-system"
database.host = "192.168.0.1"
database.user = "milter-manager"
database.password = "secret"

# Connect
database.setup

# Load definitions
database.load_models("models/*.rb")
# Operate data
User.all.each do |user|
  p user.name # => "alice", "bob", ...
end</pre>

/etc/milter-manager/models/user.rb:

<pre>class User < ActiveRecord::Base
end</pre>

Here are configuration items:

<dl>
<dt>database.type</dt>
<dd>Specifies a database type.

Here are available types:

<dl>
<dt>"mysql2"</dt>
<dd>Uses MySQL. You need to install mysql2 gem:

<pre>% sudo gem install mysql2</pre></dd>
<dt>"sqlite3"</dt>
<dd>Uses SQLite3. You need to install sqlite3 gem:

<pre>% sudo gem install sqlite3</pre></dd>
<dt>"pg"</dt>
<dd>Uses PostgreSQL. You need to install pg gem:

<pre>% sudo gem install pg</pre></dd></dl>

Example:

<pre>database.type = "mysql2" # uses MySQL</pre></dd>
<dt>database.name</dt>
<dd>Specifies database name.

For SQLite3, it specifies database path or <code>":memory:"</code>.

Example:

<pre>database.name = "configurations" # connects 'configurations' database</pre></dd>
<dt>database.host</dt>
<dd>Specifies database server host name.

In MySQL and so on, "localhost" is used as the default value.

In SQLite3, it is ignored.

Example:

<pre>database.host = "192.168.0.1" # connects to server running at 192.168.0.1</pre></dd>
<dt>database.port</dt>
<dd>Specifies database server port number.

In many cases, you don't need to specify this valueexplicitly. Because an applicable default value is used.

In SQLite3, it is ignored.

Example:

<pre>database.port = 3306 # connects to server running at3 3306 port.</pre></dd>
<dt>database.path</dt>
<dd>Specifies database server UNIX domain socket path.

In SQLite3, it is used as database path. But.#database.name is prioritize over this. It'srecommended that .#database.name is used rather than.#database.path.

Example:

<pre>database.path = "/var/run/mysqld/mysqld.sock" # connects to MySQL via UNIX domain socket</pre></dd>
<dt>database.user</dt>
<dd>Specifies database user on connect.

In SQLite3, it is ignored.

Example:

<pre>database.user = "milter-manager" # connects to server as 'milter-manager' user</pre></dd>
<dt>database.password</dt>
<dd>Specifies database password on connect.

In SQLite3, it is ignored.

Example:

<pre>database.password = "secret"</pre></dd>
<dt>database.extra_options</dt>
<dd>Since 1.6.9.

Specifies extra options. Here is an example to specify<code>:reconnect</code> option to ActiveRecord's MySQL2 adapter:

<pre>database.type = "mysql2"
database.extra_options[:reconnect] = true</pre>

Available extra options are different for each database.

Example:

<pre>database.extra_options[:reconnect] = true</pre></dd>
<dt>database.setup</dt>
<dd>Connects to database.

You connect to database at this time. After this, you canoperate data.

Example:

<pre>database.setup</pre></dd>
<dt>database.load_models(path)</dt>
<dd>Loads Ruby scripts that write class definitions forActiveRecord. You can use glob for <var>path</var>. You canuse "models/*.rb" for loading all files under models/directory. If <var>path</var> is relative path, it is resolvedfrom a directory that has milter-manager.conf.

Example:

<pre># Loads
#   /etc/milter-manager/models/user.rb
#   /etc/milter-manager/models/group.rb
#   /etc/milter-manager/models/...
# (In /etc/milter-manager/milter-manager.conf case.)
database.load_models("models/*.rb")</pre></dd></dl>


