---
title: milter-manager / milter manager / milter manager's manual
---

# milter-manager / milter manager / milter manager's manual

## NAME

milter-manager - an effective anti-spam and anti-virus solution with milters

## SYNOPSIS

<code>milter-manager</code> [<em>option ...</em>]

## DESCRIPTION

milter-manager is a milter that provides an effectiveanti-spam and anti-virus solution with milters.

milter-manager provides a platform to use milterseffectively and flexibly. milter-manager has embedded Rubyinterpreter that is used for dynamic milter applicablecondition. milter-manager can provide the platform byembedded Ruby interpreter.

milter-manager reads its configuration file. The currentconfiguration can be confirmed by --show-config option:

<pre>% milter-manager --show-config</pre>

milter-manager also provides other options that overridesconfigurations specified in configuration file.

## Options

<dl>
<dt>--help</dt>
<dd>Shows available options and exits.</dd>
<dt>--connection-spec=SPEC</dt>
<dd>Specifies a socket that accepts connections fromMTA. SPEC should be formatted as one of the followings:

* unix:PATH
* inet:PORT
* inet:PORT@HOST
* inet:PORT@[ADDRESS]
* inet6:POST
* inet6:PORT@HOST
* inet6:PORT@[ADDRESS]

Examples:

* unix:/var/run/milter/milter-manager.sock
* inet:10025
* inet:10025@localhost
* inet:10025@[127.0.0.1]
* inet6:10025
* inet6:10025@ip6-localhost
* inet6:10025@[::1]

This option overrides "manager.connection_spec" inconfiguration file.</dd>
<dt>--config-dir=DIRECTORY</dt>
<dd>Specifies a directory that includes milter-manager'sconfiguration file. milter-manager tries to loadDIRECTORY/milter-manager.conf. If it isn't find,milter-manager loads milter-manager.conf in defaultdirectory.</dd>
<dt>--pid-file=FILE</dt>
<dd>Saves process ID of milter-manager to FILE.

This option overrides "manager.pid_file" in configurationfile.</dd>
<dt>--user-name=NAME</dt>
<dd>Runs milter-manager as NAME user.milter-manager should be started by root.

This option overrides "security.effective_user" inconfiguration file.</dd>
<dt>--group-name=NAME</dt>
<dd>Runs milter-manager as NAME group.milter-manager should be started by root.

This option overrides "security.effective_group" inconfiguration file.</dd>
<dt>--socket-group-name=NAME</dt>
<dd>Changes group of UNIX domain socket for acceptingconnection by milter-manager to NAME group. Specifiedgroup should be one of the effective user's supplementarygroups.

This option overrides "manager.unix_socket_group" inconfiguration file.</dd>
<dt>--daemon</dt>
<dd>Runs milter-manager as daemon process.

This option overrides "manager.daemon" in configurationfile.</dd>
<dt>--no-daemon</dt>
<dd>This option cancels the prior --daemon option.</dd>
<dt>--show-config</dt>
<dd>Shows the current configuration and exits. The outputformat can be used in configuration file. This option isuseful for confirming registered milters and reporting yourmilter-manager's configuration when you reportmilter-manager's problems.</dd>
<dt>--log-level=LEVEL</dt>
<dd>Specifies log output items. You can specify multiple items by separatingitems with "|" like "error|warning|message".

See Log list - Level for available levels.</dd>
<dt>--log-path=PATH</dt>
<dd>Specifies log output path. If you don't specify this option, logoutput is the standard output. You can use "-" to output to thestandard output.</dd>
<dt>--event-loop-backend=BACKEDN</dt>
<dd>Uses <var>BACKEND</var> as event loop backend.Available values are <kbd>glib</kbd> or <kbd>libev</kbd>.If you use glib backend, please refer to the following note.

NOTE: For the sake of improving milter-manager performance per process,event-driven model based architecture pattern is choosed in this software.If this feature is implemented by glib, it is expressed as a callback.Note that glib's callback registration upper limit makesthe limitation of the number of communications.This limitations exist against glib backend only.</dd>
<dt>--verbose</dt>
<dd>Logs verbosely. Logs by syslog with "mail". Ifmilter-manager isn't daemon process, standard output isalso used.

"--log-level=all" option has the same effect.</dd>
<dt>--version</dt>
<dd>Shows version and exits.</dd></dl>

## EXIT STATUS

The exit status is 0 if milter starts to listen and non 0otherwise. milter-manager can't start to listen whenconnection spec is invalid format or other connectionspecific problems. e.g. the port number is already used,permission isn't granted for create UNIX domain socket andso on.

## FILES

<dl>
<dt>/usr/local/etc/milter-manager/milter-manager.conf</dt>
<dd>The default configuration file.</dd></dl>

## SIGNALS

Milter-manager processes the following signals:

<dl>
<dt>SIGHUP</dt>
<dd>Milter-manager reloads its configuration file.</dd>
<dt>SIGUSR1</dt>
<dd>Milter-manager reopenes log file.</dd></dl>

## EXAMPLE

The following example is good for debugging milter-managerbehavior. In the case, milter-manager works in theforeground and logs are outputted to the standard output.

<pre>% milter-manager --no-daemon --verbose</pre>

## SEE ALSO

milter-test-server.rd(1),milter-test-client.rd(1),milter-performance-check.rd(1),milter-manager-log-analyzer.rd(1)


