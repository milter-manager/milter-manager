---
title: Log list
---

# Log list --- List of logs by milter manager

## About this document

This document describes about logs by milter manager.

## Module

Milter manager has 4 modules as followings:

<dl>
<dt>core</dt>
<dd>This module provides common functionalities used by client, serverand manager. This module provides input/output, encode/decodefunctionalities.</dd>
<dt>client</dt>
<dd>This module provides functionalities to implement milter. Thismodule uses core module.</dd>
<dt>server</dt>
<dd>This module provides functionalities to implement MTA side milter'scommunication part. This module uses core module.</dd>
<dt>manager</dt>
<dd>This module provides functionalities to implement milter manager.This module uses core, client and server module.</dd></dl>

Milter which is implemented with libmilter-client outputs core's logand client's log because it uses client module. Milter manager outputsall modules' log because it uses all modules.

## Level {#level}

You can specify multiple log levels if you want to output multiple loglevel information.

* default: Output critical, error and warning information.
* none: Output nothing.
* critical: Output only critial information.
* error: Output only error information.
* warning: Output only warning information.
* info: Output only additional information.
* debug: Output only debug information.
* statistics: Output only statistics information.
* profile: Output only profile information.
* all: Output all log information.

## Format

Log format is as following:

<pre>[#{session_id}] [#{tag1}][#{tag2}][...] #{message}</pre>

"session_id" and "message" may be omitted.

<dl>
<dt>session_id</dt>
<dd>"session_id" is number. It is unique for each session. Use same"session_id" in same session with cross-module.</dd>
<dt>tag</dt>
<dd>Use alphabet, number, hyphen(-) and underbar(_). There is no rule intag name format.</dd>
<dt>message</dt>
<dd>Use any characters.</dd></dl>

Example (omit session_id):

<pre>[agent][error][decode] Decode error</pre>

This log has tags "agent", "error" and "decode".This log has a message "Decode error".

Example (has session_id):

<pre>[29] [agent][error][decode] Decode error</pre>

## Log List

Describe conditions to output logs for each module.

<dl>
<dt>#{level}: [#{tag1}][#{tag2}][...]</dt>
<dd>#{condition to output this log}</dd></dl>

For example,

<dl>
<dt>error: [reader][error][read]</dt>
<dd>Error occurs while reading.</dd></dl>

Output this error level log when error occurs while reading.  And ithas some tags such as "reader", "error" and "read".

### core

This is the log list of core module.  Core module has some objects asfollowings. Tag includes object name.

* reader: Data reading object.
* writer: Date writing object.
* agent: Data transfer object.Use reader to read data and use writer to write data.

<dl>
<dt>error: [reader][error][read]</dt>
<dd>Error occurs while reading.</dd>
<dt>error: [reader][callback][error]</dt>
<dd>Input error.</dd>
<dt>error: [reader][watch][read][fail]</dt>
<dd>Failure to register readable detection monitoring.</dd>
<dt>error: [reader][watch][error][fail]</dt>
<dd>Failure to register input error detection monitoring.</dd>
<dt>error: [reader][error][shutdown]</dt>
<dd>Failure to end processing of the input.</dd>
<dt>error: [writer][flush-callback][error]</dt>
<dd>Error occurs while flashing output data.</dd>
<dt>error: [writer][write-callback][error]</dt>
<dd>Error occurs while writing.</dd>
<dt>error: [writer][write][error]</dt>
<dd>Error occurs when request to write.</dd>
<dt>error: [writer][flush][error]</dt>
<dd>Error occurs when request to flash output data.</dd>
<dt>error: [writer][error-callback][error]</dt>
<dd>Output error.</dd>
<dt>error: [writer][watch][fail]</dt>
<dd>Failure to register output error detection monitoring.</dd>
<dt>error: [writer][shutdown][flush-buffer][write][error]</dt>
<dd>Error occurs when output unsent data before end processing of output.</dd>
<dt>error: [writer][shutdown][flush-buffer][flush][error]</dt>
<dd>Error occurs when flash unsent data before end processing of output.</dd>
<dt>error: [writer][error][shutdown]</dt>
<dd>Failure to end processing output.</dd>
<dt>error: [agent][error][decode]</dt>
<dd>Error occurs while analyzing input data.</dd>
<dt>error: [agent][error][reader]</dt>
<dd>Error occurs while reading input data.</dd>
<dt>error: [agent][error][writer]</dt>
<dd>Error occurs while writing output data.</dd>
<dt>error: [agent][error][set-writer][auto-flush]</dt>
<dd>Error occurs when automatic flashing while switching output destination.</dd></dl>

### client

This is the log list of client module.

There is no "session_id" in log not related to session.

<dl>
<dt>error: [client][connection-spec][default][error]</dt>
<dd>Failure to set default connection information.</dd>
<dt>error: [client][error][write]</dt>
<dd>Error occurs while writing output data.</dd>
<dt>error: [client][error][buffered-packets][write]</dt>
<dd>Error occurs while writing buffered output data.</dd>
<dt>error: [client][error][reply-on-end-of-message][flush]</dt>
<dd>Error occurs while automatic flashing when response of end-of-message event.</dd>
<dt>error: [client][error][unix]</dt>
<dd>Error occurs while initializing UNIX domain socket.</dd>
<dt>error: [client][unix][error]</dt>
<dd>Error occurs while finishing UNIX domain socket.</dd>
<dt>error: [client][single-thread][start][error]</dt>
<dd>Error occurs when start main loop in single-thread mode.</dd>
<dt>error: [client][multi-thread][start][error]</dt>
<dd>Error occurs when start main loop in multi-thread mode.</dd>
<dt>error: [client][main][error]</dt>
<dd>Error occurs when start processing.</dd>
<dt>warning: [client][accept][suspend]</dt>
<dd>Client interrupts the connection acceptance temporarily becausethere to many concurrent connections (shortage of RLIMIT_NOFILE)</dd>
<dt>warning: [client][accept][resume]</dt>
<dd>Client resumes the connection acceptance.</dd>
<dt>warning: [client][error][accept]</dt>
<dd>Failure to accept connection.</dd>
<dt>error: [client][single-thread][accept][start][error]</dt>
<dd>Failure to start connection acceptance in single-thread mode.</dd>
<dt>error: [client][multi-thread][accept][error]</dt>
<dd>Failure to start connection acceptance in multi-thread mode.</dd>
<dt>error: [client][multi-thread][error]</dt>
<dd>Error occurs while adding thread to thread pool.</dd>
<dt>error: [client][watch][error]</dt>
<dd>Error occurs on the socket is listening.</dd>
<dt>error: [client][prepare][error]</dt>
<dd>Error occurs while preparing.</dd>
<dt>error: [client][prepare][listen][error]</dt>
<dd>Failure to listen(2).</dd>
<dt>error: [client][pid-file][error][remove]</dt>
<dd>Error occurs while removing PID file.</dd>
<dt>error: [client][pid-file][save][error]</dt>
<dd>Error occurs while saving PID file.</dd>
<dt>error: [client][run][success][cleanup][error]</dt>
<dd>Error occurs when cleanup after successful completion.</dd>
<dt>error: [client][run][fail][cleanup][error]</dt>
<dd>Error occurs when cleanup after abnormal termination.</dd>
<dt>error: [client][master][run][error]</dt>
<dd>Error occurs when start processing of master process.</dd>
<dt>error: [client][worker][run][error]</dt>
<dd>Error occurs when start processing of worker process.</dd>
<dt>error: [client][worker][run][listen][error]</dt>
<dd>Error occurs when listen(2) of worker process in multi-worker mode.</dd>
<dt>error: [client][workers][run][listen][error]</dt>
<dd>Error occurs when listen(2) of master process in multi-worker mode.</dd>
<dt>warning: [client][option][deprecated]</dt>
<dd>Specify deprecated option(s).</dd>
<dt>statistics: [sessions][finished]</dt>
<dd>Output statistics as message when finish session.

<pre>#{total processed sessions}(+#{processed sessions since previous log}) #{processing sessions}</pre>

Example message:

<pre>29(+2) 3</pre></dd>
<dt>statistics: [reply][end-of-message][quarantine]</dt>
<dd>Return quarantine response.</dd></dl>

### server

This is log list of server module.

This module's log includes partner milter name because this modulecommunicate with multiple milters at the same time.

<pre>[#{session_id}] [#{tag1}][#{tag2}][...] [#{milter name}] #{message}</pre>

In addition, there is no "session_id" in log not related to session.

<dl>
<dt>error: [server][dispose][body][remained]</dt>
<dd>Remain unsent message body after the session.The milter may close connection forcibly.</dd>
<dt>error: [server][flushed][error][next-state][invalid]</dt>
<dd>Detect invalid state transition while writing data asynchronously.The milter may violate milter protocol.</dd>
<dt>error: [server][error]</dt>
<dd>Unexpected error occurs. You can read message as description.</dd>
<dt>error: [server][error][write]</dt>
<dd>Error occurs while writing.</dd>
<dt>error: [server][error][#{response}][state][invalid][#{state}]</dt>
<dd>The milter returns unexpected response when the milter state is "state".The message includes expected response list. The milter may violatemilter protocol.

"state" list is as followings:

<dl>
<dt>invalid</dt>
<dd>Invalid state. Usually, the server should not be in this state.</dd>
<dt>start</dt>
<dd>State immediately after the server starts a conversation.</dd>
<dt>define-macro</dt>
<dd>State that is communicating the macro definition.</dd>
<dt>negotiate</dt>
<dd>State that is negotiating exchange method in the session between server and milter.</dd>
<dt>connect</dt>
<dd>State that is establishing connection with milter after negotiation state.</dd>
<dt>helo</dt>
<dd>State that is processing the milter protocol corresponding to SMTP HELO.</dd>
<dt>envelope-from</dt>
<dd>State that is processing the milter protocol corresponding to SMTP FROM.</dd>
<dt>envelope-recipient</dt>
<dd>State that is processing the milter protocol corresponding to SMTP RCPT.</dd>
<dt>data</dt>
<dd>State that is processing the milter protocol corresponding to SMTP DATA.</dd>
<dt>unknown</dt>
<dd>State that is processing the milter protocol corresponding to SMTP unknown command.</dd>
<dt>header</dt>
<dd>State that is processing mail header sent via SMTP DATA by the milter protocol.</dd>
<dt>end-of-header</dt>
<dd>State that is processing notification that has finished processing mail header.</dd>
<dt>body</dt>
<dd>State that is processing mail body sent via SMTP DATA by the milter protocol.</dd>
<dt>end-of-message</dt>
<dd>State that is processing notification that has finished processing mail body.</dd>
<dt>quit</dt>
<dd>State that is processing the milter protocol quitting.</dd>
<dt>abort</dt>
<dd>State that is processing the milter protocol aborting.</dd></dl>

Response from the milter is as following.Each item is corresponding to milter protocol response.

<dl>
<dt>negotiate-reply</dt>
<dd>Response negotiate.</dd>
<dt>continue</dt>
<dd>Response that represents the continuation of the process.</dd>
<dt>reply-code</dt>
<dd>Response that represents to specify SMTP response code.</dd>
<dt>add-header</dt>
<dd>Response that represents to append header.</dd>
<dt>insert-header</dt>
<dd>Response that represents to insert header into any position.</dd>
<dt>change-header</dt>
<dd>Response that represents to change header.</dd>
<dt>add-recipient</dt>
<dd>Response that represents to change recipient.</dd>
<dt>delete-recipient</dt>
<dd>Response that represents to delte recipient.</dd>
<dt>replace-body</dt>
<dd>Response that represents to replace body.</dd>
<dt>progress</dt>
<dd>Response that represents to be in progress.This response is used to increase the timeout.</dd>
<dt>quarantine</dt>
<dd>Response that represents to quarantine the mail.</dd>
<dt>skip</dt>
<dd>Response that represents to skip receiving body.</dd></dl></dd>
<dt>error: [server][timeout][connection]</dt>
<dd>Connection timeout.</dd>
<dt>error: [server][error][connect]</dt>
<dd>Connection error.</dd>
<dt>error: [server][error][connected][start]</dt>
<dd>Error occurs while initializing to start connecting.</dd>
<dt>warning: [server][reply][quitted][#{state}][#{response}]</dt>
<dd>Server receives a response after finished milter protocol."state" and "response" are same as "[server][error][#{response}][state][invalid][#{state}]".No problem if there are no error log after this log.</dd></dl>

### manager

This is log list of manager module.

There is no "session_id" in log not related to session.

#### submodule

First tag is submodule name in manager module. However, there is notag when milter-manager crashes in manager module and whenmilter-manager is booting.

Submodules are 8 items as following:

* manager
* configuration
* launcher
* process-launcher
* controller
* leader
* children
* egg

<dl>
<dt>manager</dt>
<dd>This submodule implements milter-manager command using each submodules.</dd>
<dt>configuration</dt>
<dd>This submodule loads configuration file.</dd>
<dt>launcher</dt>
<dd>This submodule launches child-milter.</dd>
<dt>process-launcher</dt>
<dd>This submodule launches launcher submodule in milter-manager command.</dd>
<dt>controller</dt>
<dd>This submodule operates milter-manager process from the outside.</dd>
<dt>leader</dt>
<dd>This submodule returns result that children module collect allchild-milters' result to MTA.</dd>
<dt>children</dt>
<dd>This submodule collect and summarize multiple child-milters' result.</dd>
<dt>egg</dt>
<dd>This module manages information of one child-milter.</dd></dl>

Describe for each submodule.

#### submodule: manager

<dl>
<dt>error: [manager][reload][signal][error]</dt>
<dd>Error occurs while reloading configuration requested by SIGHUP signal.</dd></dl>

#### submodule: configuration

<dl>
<dt>error: [configuration][dispose][clear][error]</dt>
<dd>Fail to clear configuration while disposing.</dd>
<dt>error: [configuration][new][clear][error]</dt>
<dd>Fail to clear configuration at the first time.</dd>
<dt>error: [configuration][load][clear][error]</dt>
<dd>Fail to clear configuration before reload configuration.</dd>
<dt>error: [configuration][load][error]</dt>
<dd>Fail to load milter-manager.conf.</dd>
<dt>error: [configuration][load][custom][error]</dt>
<dd>Fail to load milter-manager.custom.conf.</dd>
<dt>error: [configuration][clear][custom][error]</dt>
<dd>Fail to clear configuration using Ruby.</dd>
<dt>error: [configuration][maintain][error]</dt>
<dd>Error occurs while processing of maintenance every maitenance_interval seconds.See manager.maintenance_interval.</dd>
<dt>error: [configuration][event-loop-created][error]</dt>
<dd>Error occurs after create event loop.</dd></dl>

#### submodule: launcher

<dl>
<dt>error: [launcher][error][child][authority][group]</dt>
<dd>Fail to change execution privilege to specified group when launchchild-milter.</dd>
<dt>error: [launcher][error][child][authority][groups]</dt>
<dd>Fail to initialize specified additional group when launch child-milter.</dd>
<dt>error: [launcher][error][child][authority][user]</dt>
<dd>Fail to change execution privilege to specified user when launch child-milter.</dd>
<dt>error: [launcher][error][launch]</dt>
<dd>Fail to launch child-milter.</dd>
<dt>error: [launcher][error][write]</dt>
<dd>Error occurs when this module writes response whether success or not.</dd></dl>

#### submodule: process-launcher

<dl>
<dt>error: [process-launcher][error][start]</dt>
<dd>Error occurs when this module start connecting with launcher module.</dd>
<dt>error: [process-launcher][error]</dt>
<dd>Fail to detach file descriptors from forked launcher module.</dd></dl>

#### submodule: controller

<dl>
<dt>error: [controller][error][write][success]</dt>
<dd>Error occurs when this module writes successful response.</dd>
<dt>error: [controller][error][write][error]</dt>
<dd>Error occurs when this module writes error response.</dd>
<dt>error: [controller][error][save]</dt>
<dd>Error occurs when this module saves configuration file.</dd>
<dt>error: [controller][error][write][configuration]</dt>
<dd>Error occurs when this module writes configuration file.</dd>
<dt>error: [controller][reload][error]</dt>
<dd>Error occurs when this module reloads configuration.</dd>
<dt>error: [controller][error][write][status]</dt>
<dd>Error occurs when this module writes status response.</dd>
<dt>error: [controller][error][unix]</dt>
<dd>Error occurs when this module deletes UNIX domain socket if enabled remove_unix_socket_on_create.See controller.remove_unix_socket_on_create.</dd>
<dt>error: [controller][error][start]</dt>
<dd>Fail to start communication.</dd>
<dt>error: [controller][error][accept]</dt>
<dd>Fail to accept connection.</dd>
<dt>error: [controller][error][watch]</dt>
<dd>Error occurs while communicating.</dd>
<dt>error: [controller][error][listen]</dt>
<dd>Fail to call listen(2).</dd></dl>

#### submodule: leader

<dl>
<dt>error: [leader][error][invalid-state]</dt>
<dd>Detect invalid state transition. Probably this is milter manager's bug.</dd>
<dt>error: [leader][error]</dt>
<dd>Error occurs in children module.</dd>
<dt>error: [leader][error][reply-code]</dt>
<dd>Fail to specify SMTP response code. Milter may send invalid response code.</dd>
<dt>error: [leader][error][add-header]</dt>
<dd>Error occurs when append header.</dd>
<dt>error: [leader][error][insert-header]</dt>
<dd>Error occurs when insert header into any position.</dd>
<dt>error: [leader][error][delete-header]</dt>
<dd>Error occurs when delete header.</dd>
<dt>error: [leader][error][change-from]</dt>
<dd>Error occurs when change recipient.</dd>
<dt>error: [leader][error][add-recipient]</dt>
<dd>Error occurs when add recipient.</dd>
<dt>error: [leader][error][delete-recipient]</dt>
<dd>Error occurs when delete recipient.</dd>
<dt>error: [leader][error][replace-body]</dt>
<dd>Error occurs when replace body.</dd></dl>

#### submodule: children

<dl>
<dt>error: [children][error][negotiate]</dt>
<dd>This module receives response though this module does not sendrequest to start negotiation. Milter may violate milter protocol.</dd>
<dt>error: [children][error][pending-message-request]</dt>
<dd>A request to child-milter has a problem when milter-manager processdelays it. If you find this log, milter manager has a bug.</dd>
<dt>error: [children][error][body][read][seek]</dt>
<dd>Error occurs when seek position to read body.</dd>
<dt>error: [children][error][body][read]</dt>
<dd>Error occurs while reading body.</dd>
<dt>error: [children][error][invalid-state][temporary-failure]</dt>
<dd>Milter respond "temporary-failure" response at unexpected timing inmilter protocol. Milter may violate milter protocol.</dd>
<dt>error: [children][error][invalid-state][reject]</dt>
<dd>Milter respond "reject" response at unexpected timing inmilter protocol. Milter may violate milter protocol.</dd>
<dt>error: [children][error][invalid-state][accept]</dt>
<dd>Milter respond "accept" response at unexpected timing inmilter protocol. Milter may violate milter protocol.</dd>
<dt>error: [children][error][invalid-state][discard]</dt>
<dd>Milter respond "discard" response at unexpected timing inmilter protocol. Milter may violate milter protocol.</dd>
<dt>error: [children][error][invalid-state][stopped]</dt>
<dd>milter-manager process stops child-milter at unexpected timing inmilter protocol. Probably this is milter manager's bug.</dd>
<dt>error: [children][error][invalid-state][#{response}][#{state}]</dt>
<dd>Milter can return "#{response}" at state of "end-of-message". However milterreturns "#{response}" at state of "#{state}".Milter may violate milter protocol.

"#{response}" is as following:

<dl>
<dt>add-header</dt>
<dd>Response represents to append header.</dd>
<dt>insert-header</dt>
<dd>Response represents to insert header into any position.</dd>
<dt>change-header</dt>
<dd>Response represents to change header.</dd>
<dt>delete-header</dt>
<dd>Response represents to delete header.</dd>
<dt>change-from</dt>
<dd>Response represents to change from.</dd>
<dt>add-recipient</dt>
<dd>Response represents to add recipient.</dd>
<dt>delete-recipient</dt>
<dd>Response represents to delete recipient.</dd>
<dt>replace-body</dt>
<dd>Response represents to replace body.</dd>
<dt>progress</dt>
<dd>Response represents process is in progress.</dd>
<dt>quarantine</dt>
<dd>Response represents to quarantine a mail.</dd></dl>

"#{state}" is same as "[server][error][#{response}][state][invalid][#{state}]".</dd>
<dt>error: [children][timeout][writing]</dt>
<dd>Writing timeout.</dd>
<dt>error: [children][timeout][reading]</dt>
<dd>Reading timeout.</dd>
<dt>error: [children][timeout][end-of-message]</dt>
<dd>Response timeout when "end-of-message".</dd>
<dt>error: [children][error][#{state}][#{fallback_status}]</dt>
<dd>Error occurs in server module that is used communication with child-milter.See milter.fallback_status about child-milter result.See "[server][error][#{response}][state][invalid][#{state}]" abount "#{state}".</dd>
<dt>error: [children][error][start-child][write]</dt>
<dd>Error occurs when this module requests launcer submodule to lauchchild-milter.</dd>
<dt>error: [children][error][start-child][flush]</dt>
<dd>Error occurs when this module flashes request that startschild-milter to launcher submodule.</dd>
<dt>error: [children][error][negotiate][not-started]</dt>
<dd>This module executes process of negotiation response but MTA doesnot start negotiation. Probably milter manager has a bug if youfind this log.</dd>
<dt>error: [children][error][negotiate][no-response]</dt>
<dd>All milters do not respond to negotiation. Probably child-milterhas some problems if you find this log.</dd>
<dt>error: [children][timeout][connection]</dt>
<dd>Connection timeout.</dd>
<dt>error: [children][error][connection]</dt>
<dd>Error occurs while processing connection.</dd>
<dt>error: [children][error][alive]</dt>
<dd>MTA send requests to this module though all child-milter havefinished.</dd>
<dt>error: [children][error][message-processing]</dt>
<dd>MTA send request that process mail to this module though nochild-milter processes data which is sent via SMTP DATA.</dd>
<dt>error: [children][error][body][open]</dt>
<dd>Fail to create temporary file to save large mail body.</dd>
<dt>error: [children][error][body][encoding]</dt>
<dd>Fail to set encoding to read/write temporary file to save largemail body.</dd>
<dt>error: [children][error][body][write]</dt>
<dd>Fail to write data to temporary file to save large mail body.</dd>
<dt>error: [children][error][body][send][seek]</dt>
<dd>Fail to seek position in temporary file to save large mail body.</dd>
<dt>error: [children][error][body][send]</dt>
<dd>Fail to read data from temporary file to save large mail body.</dd></dl>

#### submodule: egg

<dl>
<dt>error: [egg][error]</dt>
<dd>Detect invalid connection spec is specified when this moduleconnect to child-milter.</dd>
<dt>error: [egg][error][set-spec]</dt>
<dd>Set invalid connection spec to child-milter.</dd></dl>

#### other: logs when crashed

<dl>
<dt>critical: [#{signal}] unable to open pipe for collecting stack trace</dt>
<dd>Unable to open pipe for collecting stack trace.Signal is SEGV or ABORT.</dd>
<dt>critical: #{stack_trace}</dt>
<dd>No tags. Display stack trace for each line.</dd></dl>

#### other: starting milter-manager command

<dl>
<dt>error: failed to create pipe for launcher command</dt>
<dd>Failed to create pipe for launcher command.</dd>
<dt>error: failed to create pipe for launcher reply</dt>
<dd>Failed to create pipe for launcher reply.</dd>
<dt>error: failed to fork process launcher process</dt>
<dd>Failed to fork process launcher process.</dd>
<dt>error: failed to find password entry for effective user</dt>
<dd>Failed to find password entry for effective user.</dd>
<dt>error: failed to get password entry for effective user</dt>
<dd>Failed to get password entry for effective user.</dd>
<dt>error: failed to get limit for RLIMIT_NOFILE</dt>
<dd>Failed to get limit for RLIMIT_NOFILE.</dd>
<dt>error: failed to set limit for RLIMIT_NOFILE</dt>
<dd>Failed to set limit for RLIMIT_NOFILE.</dd>
<dt>error: failed to create custom configuration directory</dt>
<dd>Failed to create custom configuration directory.</dd>
<dt>error: failed to change owner and group of configuration directory</dt>
<dd>Failed to change owner and group of configuration directory</dd>
<dt>error: failed to listen</dt>
<dd>Failed to listen(2).</dd>
<dt>error: failed to drop privilege</dt>
<dd>Failed to drop root privilege.</dd>
<dt>error: failed to listen controller socket:</dt>
<dd>Failed to listen(2) controller socket.</dd>
<dt>error: failed to daemonize:</dt>
<dd>Failed to daemonize.</dd>
<dt>error: failed to start milter-manager process:</dt>
<dd>Failed to start milter-manager process.</dd>
<dt>error: [manager][reload][custom-load-path][error]</dt>
<dd>Failed to reload configuration.</dd>
<dt>error: [manager][configuration][reload][command-line-load-path][error]</dt>
<dd>Failed to load configuration from load path specified by command line.</dd></dl>

#### other: statistics

<dl>
<dt>statistics: [milter][header][add]</dt>
<dd>Child-milter adds header.</dd>
<dt>statistics: [milter][end][#{last_state}][#{status}][#{elapsed}]</dt>
<dd>One child-milter has finished.

"[#{last_state}]" represents state that it has finished.See "#{state}" of "[server][error][#{response}][state][invalid][#{state}]".

"#{status}" is response from child-milter. Response is as following.

<dl>
<dt>reject</dt>
<dd>Response represents to reject.</dd>
<dt>discard</dt>
<dd>Response represents to discard.</dd>
<dt>accept</dt>
<dd>Response represents to accept.</dd>
<dt>temporary-failure</dt>
<dd>Response represents temporary failure.</dd>
<dt>pass</dt>
<dd>Response represents to pass implicitly.</dd>
<dt>stop</dt>
<dd>Response represents to stop process in progress..</dd></dl>

"[#{elapsed}]" represents elapsed time since connection start.</dd>
<dt>statistics: [session][end][#{last_state}][#{status}][#{elapsed}]</dt>
<dd>One milter session has finished.See "[milter][end][#{last_state}][#{status}][#{elapsed}]".</dd>
<dt>statistics: [session][disconnected][#{elapsed}]</dt>
<dd>SMTP client has disconnected the session in progress.

"[#{elapsed}]" represents elapsed time since connection start.</dd>
<dt>statistics: [session][header][add]</dt>
<dd>Add header as response of whole milter session.</dd></dl>


