---
title: milter-test-client / milter manager / milter manager's manual
---

# milter-test-client / milter manager / milter manager's manual

## NAME

milter-test-client - milter side milter protocol implemented program

## SYNOPSIS

<code>milter-test-client</code> [<em>option ...</em>]

## DESCRIPTION

milter-test-client is a milter that just shows received datafrom MTA. It also shows macros received from MTA, it can beused for confirming MTA's milter configuration.

Postfix's source archive includes similar tool.It's src/milter/test-milter.c. It seems that it's used fortesting Postfix's milter implementation. But test-milterdoesn't show macros. If you have a milter that doesn't workas you expect and uses macro, milter-test-client is usefultool for looking into the problems.

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

* unix:/tmp/milter-test-client.sock
* inet:10025
* inet:10025@localhost
* inet:10025@[127.0.0.1]
* inet6:10025
* inet6:10025@localhost
* inet6:10025@[::1]</dd>
<dt>--log-level=LEVEL</dt>
<dd>Specifies log output items. You can specify multiple items by separatingitems with "|" like "error|warning|message".

See Log list - Level for available levels.</dd>
<dt>--log-path=PATH</dt>
<dd>Specifies log output path. If you don't specify this option, logoutput is the standard output. You can use "-" to output to thestandard output.</dd>
<dt>--verbose</dt>
<dd>Logs verbosely.

"--log-level=all" option has the same effect.</dd>
<dt>--syslog</dt>
<dd>Logs Syslog too.</dd>
<dt>--no-report-request</dt>
<dd>Doesn't show any information received from MTA.</dd>
<dt>--report-memory-profile</dt>
<dd>Reports memory usage each milter session finished.

When MILTER_MEMORY_PROFILE environment variable is set to'yes', details are reported.

Example:

<pre>% MILTER_MEMORY_PROFILE=yes milter-test-client -s inet:10025</pre></dd>
<dt>--daemon</dt>
<dd>Runs as daemon process.</dd>
<dt>--user=USER</dt>
<dd>Runs as USER's process. root privilege is needed.</dd>
<dt>--group=GROUP</dt>
<dd>Runs as GROUP's process. root privilege is needed.</dd>
<dt>--unix-socket-group=GROUP</dt>
<dd>Changes UNIX domain socket group to GROUP when"unix:PATH" format SPEC is used.</dd>
<dt>--n-workers=N_WORKERS</dt>
<dd>Runs <var>N_WORKERS</var> processes to process mails.Available value is between 0 and 1000.If it is 0, no worker processes will be used.

<em>NOTE: This item is an experimental feature.</em></dd>
<dt>--event-loop-backend=BACKEND</dt>
<dd>Uses <var>BACKEND</var> as event loop backend.Available values are <kbd>glib</kbd> or <kbd>libev</kbd>.If you use glib backend, please refer to the following note.

<em>NOTE: For the sake of improving milter-manager performance per process,event-driven model based architecture pattern is choosed in this software.If this feature is implemented by glib, it is expressed as a callback.Note that glib's callback registration upper limit makesthe limitation of the number of communications.This limitations exist against glib backend only.</em></dd>
<dt>--packet-buffer-size=SIZE</dt>
<dd>Uses <var>SIZE</var> as send packets buffer size onend-of-message. Buffered packets are sent when buffersize is rather than <var>SIZE</var> bytes. Buffering isdisabled when <var>SIZE</var> is 0.

The default is 0KB. It means packet buffering is disabledby default.</dd>
<dt>--version</dt>
<dd>Shows version and exits.</dd></dl>

## EXIT STATUS

The exit status is 0 if milter starts to listen and non 0otherwise. milter-test-client can't start to listen whenconnection spec is invalid format or other connectionspecific problems. e.g. the port number is already used,permission isn't granted for create UNIX domain socket andso on.

## EXAMPLE

The following example runs a milter which listens at 10025port and waits a connection from MTA.

<pre>% milter-test-client -s inet:10025</pre>

## SEE ALSO

milter-test-server.rd(1),milter-performance-check.rd(1)


