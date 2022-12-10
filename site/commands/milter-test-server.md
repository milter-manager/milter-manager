---
title: milter-test-server / milter manager / milter manager's manual
---

# milter-test-server / milter manager / milter manager's manual

## NAME

milter-test-server - MTA side milter protocol implemented program

## SYNOPSIS

<code>milter-test-server</code> [<em>option ...</em>]

## DESCRIPTION

milter-test-server talks MTA side milter protocol. It canconnect to a milter without MTA. For now, there is nosimilar tool. It is useful to test milter not MTA +milter. For example, it can be used for the followingsituation:

* milter's performance check
* milter's operation check

milter-test-server can be used for simple performance checkbecause it shows elapsed time. You can confirm elapsed timewithout MTA's processing time. You will find a problem of amilter more easily because it doesn't depend on MTA.

If a milter changes headers and/or body, milter-test-servercan show changed message. It can be used for testing amilter that may change headers and/or body. If it is usedwith [unit testing framework likeCutter](http://cutter.sourceforge.net/), you can writeautomated unit tests.

## Options

<dl>
<dt>--help</dt>
<dd>Shows available options and exits.</dd>
<dt>--name=NAME</dt>
<dd>Uses NAME as milter-test-server's name. The name is usedas a value of "{daemon_name}" macro for example.

The default value is "milter-test-server" that is thecommand file name.</dd>
<dt>--connection-spec=SPEC</dt>
<dd>Specifies a socket to connect to milter. SPEC should beformatted as one of the followings:

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
* inet6:10025@localhost
* inet6:10025@[::1]</dd>
<dt>--negotiate-version=VERSION</dt>
<dd>Uses VERSION as milter protocol version sent to milter.

The default value is 8. The value is the same as Sendmail8.14's default value.</dd>
<dt>--connect-host=HOST</dt>
<dd>Uses HOST as connected host.

The host name is passed to milter's xxfi_connect() callback.</dd>
<dt>--connect-address=SPEC</dt>
<dd>Uses SPEC as connected address. SPEC format is same as--connection-spec option's SPEC.

The address is passed to milter's xxfi_connect() callback.</dd>
<dt>--connect-macro=NAME:VALUE</dt>
<dd>Adds a macro that is sent on xxfi_connect() callback. Themacro has NAME name and VALUE value. This option can bespecified N-times for N additional macros.

Here is an example that a macro that has"client_connections" name and "1" value is sent onxxfi_connect() callback:

<pre>--connect-macro client_connections:1</pre></dd>
<dt>--helo-fqdn=FQDN</dt>
<dd>Uses FQDN for 'HELO/EHLO' SMTP command.

The FQDN is passed to milter's xxfi_helo() callback.</dd>
<dt>--helo-macro=NAME:VALUE</dt>
<dd>Adds a macro that is sent on xxfi_helo() callback. Themacro has NAME name and VALUE value. This option can bespecified N-times for N additional macros.

Here is an example that a macro that has"client_ptr" name and "unknown" value is sent onxxfi_helo() callback:

<pre>--helo-macro client_ptr:unknown</pre></dd>
<dt>--envelope-from=FROM, -fFROM</dt>
<dd>Uses FROM for 'MAIL FROM' SMTP command.

The address is passed to milter's xxfi_envfrom() callback.</dd>
<dt>--envelope-from-macro=NAME:VALUE</dt>
<dd>Adds a macro that is sent on xxfi_envfrom() callback. Themacro has NAME name and VALUE value. This option can bespecified N-times for N additional macros.

Here is an example that a macro that has"client_addr" name and "192.168.0.1" value is sent onxxfi_envfrom() callback:

<pre>--envelope-from-macro client_addr:192.168.0.1</pre></dd>
<dt>--envelope-recipient=RECIPIENT, -rRECIPIENT</dt>
<dd>Uses RECIPIENT for 'RCPT TO' SMTP command. If you wantto use multiple recipients, specify --envelope-recipientoption n-times.

The address is passed to milter's xxfi_envrcpt()callback. xxfi_envrcpt() is called for each recipient.</dd>
<dt>--envelope-recipient-macro=NAME:VALUE</dt>
<dd>Adds a macro that is sent on xxfi_envrcpt() callback. Themacro has NAME name and VALUE value. This option can bespecified N-times for N additional macros.

Here is an example that a macro that has"client_ptr" name and "2929" value is sent onxxfi_envrcpt() callback:

<pre>--envelope-recipient-macro client_ptr:2929</pre></dd>
<dt>--data-macro=NAME:VALUE</dt>
<dd>Adds a macro that is sent on xxfi_data() callback. Themacro has NAME name and VALUE value. This option can bespecified N-times for N additional macros.

Here is an example that a macro that has"client_name" name and "unknown" value is sent onxxfi_data() callback:

<pre>--data-macro client_name:unknown</pre></dd>
<dt>--header=NAME:VALUE</dt>
<dd>Adds a header that names NAME and its value is VALUE. Ifyou want to multiple headers, specify --header optionn-times.

The header is passed to milter's xxfi_header() callback.xxfi_header() is called for each header.</dd>
<dt>--end-of-header-macro=NAME:VALUE</dt>
<dd>Adds a macro that is sent on xxfi_eoh() callback. Themacro has NAME name and VALUE value. This option can bespecified N-times for N additional macros.

Here is an example that a macro that has"n_headers" name and "100" value is sent onxxfi_eoh() callback:

<pre>--end-of-header-macro n_headers:100</pre></dd>
<dt>--body=CHUNK</dt>
<dd>Adds CHUNK as body chunk. If you want to multiplechunks, specify --body option n-times.

The chunk is passed to milter's xxfi_body()callback. xxfi_body() is called for each chunk.</dd>
<dt>--end-of-message-macro=NAME:VALUE</dt>
<dd>Adds a macro that is sent on xxfi_eom() callback. Themacro has NAME name and VALUE value. This option can bespecified N-times for N additional macros.

Here is an example that a macro that has"elapsed" name and "0.29" value is sent onxxfi_eom() callback:

<pre>--end-of-message-macro elapsed:0.29</pre></dd>
<dt>--unknown=COMMAND</dt>
<dd>Uses COMMAND as unknown SMTP command.

The command is passed to milter's xxfi_unknown()callback. xxfi_unknown() is called betweenxxfi_envrcpt() and xxfi_data().</dd>
<dt>--authenticated-name=NAME</dt>
<dd>Uses <var>NAME</var> as an authorized user name on SMTPAuth. It corresponds to SASL login name. <var>NAME</var> ispassed as a value of <code>{auth_authen}</code> on MAIL FROM.</dd>
<dt>--authenticated-type=TYPE</dt>
<dd>Uses <var>TYPE</var> as an authorized type on SMTP Auth. Itcorresponds to SASL login method. <var>TYPE</var> is passedas a value of <code>{auth_type}</code> on MAIL FROM.</dd>
<dt>--authenticated-author=AUTHOR</dt>
<dd>Uses <var>AUTHOR</var> as an authorized sender on SMTPAuth. It corresponds to SASL sender. <var>AUTHOR</var> ispassed as a value of <code>{auth_author}</code> on MAIL FROM.</dd>
<dt>--mail-file=PATH</dt>
<dd>Uses file exists at <var>PATH</var> as mail content. If the filehas 'From:' and/or 'To:', they are used for from and/orrecipient addresses.</dd>
<dt>--output-message</dt>
<dd>Shows a message applied a milter. If you want tocheck milter's operation that may change header and/orbody, specify this option.</dd>
<dt>--color=[yes|true|no|false|auto]</dt>
<dd>Shows a messaged applied a milter with colorization if--color, --color=yes or --color=true is specified. If--color=auto is specified, colorization is enabled onterminal environment.

The default is off.</dd>
<dt>--connection-timeout=SECONDS</dt>
<dd>Specifies timeout on connecting to a milter.An error is occurred when a connection can't beestablished in <var>SECONDS</var> seconds.

The default is 300 seconds. (5 minutes)</dd>
<dt>--reading-timeout=SECONDS</dt>
<dd>Specifies timeout on reading a response from a milter.An error is occurred when the milter doesn't respond to arequest in <var>SECONDS</var> seconds.

The default is 10 seconds.</dd>
<dt>--writing-timeout=SECONDS</dt>
<dd>Specifies timeout on writing a request to a milter.An error is occurred when request to the milter isn'tcompleted in <var>SECONDS</var> seconds.

The default is 10 seconds.</dd>
<dt>--end-of-message-timeout=SECONDS</dt>
<dd>Specifies timeout on reading a response ofend-of-message command from a milter.An error is occurred when the milter doesn't complete itsresponse to the end-of-message command in <var>SECONDS</var>seconds.

The default is 300 seconds. (5 minutes)</dd>
<dt>--all-timeouts=SECONDS</dt>
<dd>Specifies timeout to --connection-timeout, --reading-timeout, --writing-timeoutand --end-of-message-timeout at once.</dd>
<dt>--threads=N</dt>
<dd>Use N threads to request a milter.

The default is 0. (main thread only)</dd>
<dt>--verbose</dt>
<dd>Logs verbosely.

"MILTER_LOG_LEVEL=all" environment variable configurationhas the same effect.</dd>
<dt>--version</dt>
<dd>Shows version and exits.</dd></dl>

## EXIT STATUS

The exit status is 0 if milter session is started and non 0otherwise. milter session can't be started when connectionspec is invalid format or milter-test-server can't connectto a milter.

## EXAMPLE

The following example talks with a milter that works on host192.168.1.29 and is listened at 10025 port.

<pre>% milter-test-server -s inet:10025@192.168.1.29</pre>

## SEE ALSO

milter-test-client.rd(1),milter-performance-check.rd(1)


