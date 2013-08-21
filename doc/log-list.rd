# -*- rd -*-

= Log list --- List of logs by milter manager

== About this document

This document describes about logs by milter manager.

== Module

Milter manager has 4 modules as followings:

: core
  This module provides common functionalities used by client, server
  and manager. This module provides input/output, encode/decode
  functionalities.

: client
  This module provides functionalities to implement milter. This
  module uses core module.

: server
  This module provides functionalities to implement MTA side milter's
  communication part. This module uses core module.

: manager
  This module provides functionalities to implement milter manager.
  This module uses core, client and server module.

Milter which is implemented with libmilter-client outputs core's log
and client's log because it uses client module. Milter manager outputs
all modules' log because it uses all modules.

== Level

You can specify multiple log levels if you want to output multiple log
level information.

  * default: Output critical, error and warning.
  * none: Output nothing.
  * critical: Output critial only.
  * error: Output error only.
  * warning: Output warning only.
  * info: Output info only.
  * debug: Output debug only.
  * statistics: Output statistics only.
  * all: Output all log levels.

== Format

Log format is as following:

  [#{session_id}] [#{tag1}][#{tag2}][...] #{message}

"session_id" and "message" may be omitted.

: session_id
  "session_id" is number. It is unique for each session. Use same
  "session_id" in same session with cross-module.

: tag
  Use alphabet, number, hyphen(-) and underbar(_). There is no rule in
  tag name format.

: message
  Use any characters.

Example (omit session_id):

  [agent][error][decode] Decode error

This log has tags "agent", "error" and "decode".
This log has a message "Decode error".

Example (has session_id):

  [29] [agent][error][decode] Decode error


== Log List

Describe conditions to output logs for each module.

: #{level}: [#{tag1}][#{tag2}][...]
  #{condition to output this log}

For example,

: error: [reader][error][read]
  Error occurs while reading.

Output this error level log when error occurs while reading.  And it
has some tags such as "reader", "error" and "read".

=== core

This is the log list of core module.  Core module has some objects as
followings. Tag includes object name.

  * reader: Data reading object.
  * writer: Date writing object.
  * agent: Data transfer object.Use reader to read data and use writer to write data.

: error: [reader][error][read]

   Error occurs while reading.

: error: [reader][callback][error]

   Input error.

: error: [reader][watch][read][fail]

   Failure to register readable detection monitoring.

: error: [reader][watch][error][fail]

   Failure to register input error detection monitoring.

: error: [reader][error][shutdown]

   Failure to end processing of the input.

: error: [writer][flush-callback][error]

   Error occurs while flashing output data.

: error: [writer][write-callback][error]

   Error occurs while writing.

: error: [writer][write][error]

   Error occurs when request to write.

: error: [writer][flush][error]

   Error occurs when request to flash output data.

: error: [writer][error-callback][error]

   Output error.

: error: [writer][watch][fail]

   Failure to register output error detection monitoring.

: error: [writer][shutdown][flush-buffer][write][error]

   Error occurs when output unsent data before end processing of output.

: error: [writer][shutdown][flush-buffer][flush][error]

   Error occurs when flash unsent data before end processing of output.

: error: [writer][error][shutdown]

   Failure to end processing output.

: error: [agent][error][decode]

   Error occurs while analyzing input data.

: error: [agent][error][reader]

   Error occurs while reading input data.

: error: [agent][error][writer]

   Error occurs while writing output data.

: error: [agent][error][set-writer][auto-flush]

   Error occurs when automatic flashing while switching output destination.

=== client

This is the log list of client module.

There is no "session_id" in log not related to session.

: error: [client][connection-spec][default][error]

   Failure to set default connection information.

: error: [client][error][write]

   Error occurs while writing output data.

: error: [client][error][buffered-packets][write]

   Error occurs while writing buffered output data.

: error: [client][error][reply-on-end-of-message][flush]

   Error occurs while automatic flashing when response of end-of-message event.

: error: [client][error][unix]

   Error occurs while initializing UNIX domain socket.

: error: [client][unix][error]

   Error occurs while finishing UNIX domain socket.

: error: [client][single-thread][start][error]

   Error occurs when start main loop in single-thread mode.

: error: [client][multi-thread][start][error]

   Error occurs when start main loop in multi-thread mode.

: error: [client][main][error]

   Error occurs when start processing.

: warning: [client][accept][suspend]

   Client interrupts the connection acceptance temporarily because
   there to many concurrent connections (shortage of RLIMIT_NOFILE)

: warning: [client][accept][resume]

   Client resumes the connection acceptance.

: warning: [client][error][accept]

   Failure to accept connection.

: error: [client][single-thread][accept][start][error]

   Failure to start connection acceptance in single-thread mode.

: error: [client][multi-thread][accept][error]

   Failure to start connection acceptance in multi-thread mode.

: error: [client][multi-thread][error]

   Error occurs while adding thread to thread pool.

: error: [client][watch][error]

   Error occurs on the socket is listening.

: error: [client][prepare][error]

   Error occurs while preparing.

: error: [client][prepare][listen][error]

   Failure to listen(2).

: error: [client][pid-file][error][remove]

   Error occurs while removing PID file.

: error: [client][pid-file][save][error]

   Error occurs while saving PID file.

: error: [client][run][success][cleanup][error]

   Error occurs when cleanup after successful completion.

: error: [client][run][fail][cleanup][error]

   Error occurs when cleanup after abnormal termination.

: error: [client][master][run][error]

   Error occurs when start processing of master process.

: error: [client][worker][run][error]

   Error occurs when start processing of worker process.

: error: [client][worker][run][listen][error]

   Error occurs when listen(2) of worker process in multi-worker mode.

: error: [client][workers][run][listen][error]

   Error occurs when listen(2) of master process in multi-worker mode.

: warning: [client][option][deprecated]

   Specify deprecated option(s).

: statistics: [sessions][finished]

   Output statistics as message when finish session.

     #{total processed sessions}(+#{processed sessions since previous log}) #{processing sessions}

   Example message:

     29(+2) 3

: statistics: [reply][end-of-message][quarantine]

   Return quarantine response.

=== server

This is log list of server module.

This module's log includes partner milter name because this module
communicate with multiple milters at the same time.

  [#{session_id}] [#{tag1}][#{tag2}][...] [#{milter name}] #{message}

In addition, there is no "session_id" in log not related to session.


: error: [server][dispose][body][remained]

   Remain unsent message body after the session.
   The milter may close connection forcibly.

: error: [server][flushed][error][next-state][invalid]

   Detect invalid state transition while writing data asynchronously.
   The milter may violate milter protocol.

: error: [server][error]

   Unexpected error occurs. You can read message as description.

: error: [server][error][write]

   Error occurs while writing.

: error: [server][error][#{response}][state][invalid][#{state}]

   The milter returns unexpected response when the milter state is "state".
   The message includes expected response list. The milter may violate
   milter protocol.

   "state" list is as followings:

   : invalid
      Invalid state. Usually, the server should not be in this state.
   : start
      State immediately after the server starts a conversation.
   : define-macro
      State that is communicating the macro definition.
   : negotiate
      State that is negotiating exchange method in the session between server and milter.
   : connect
      State that is establishing connection with milter after negotiation state.
   : helo
      State that is processing the milter protocol corresponding to SMTP HELO.
   : envelope-from
      State that is processing the milter protocol corresponding to SMTP FROM.
   : envelope-recipient
      State that is processing the milter protocol corresponding to SMTP RCPT.
   : data
      State that is processing the milter protocol corresponding to SMTP DATA.
   : unknown
      State that is processing the milter protocol corresponding to SMTP unknown command.
   : header
      State that is processing mail header sent via SMTP DATA by the milter protocol.
   : end-of-header
      State that is processing notification that has finished processing mail header.
   : body
      State that is processing mail body sent via SMTP DATA by the milter protocol.
   : end-of-message
      State that is processing notification that has finished processing mail body.
   : quit
      State that is processing the milter protocol quitting.
   : abort
      State that is processing the milter protocol aborting.

   Response from the milter is as following.
   Each item is corresponding to milter protocol response.

   : negotiate-reply
      Response negotiate.
   : continue
      Response that represents the continuation of the process.
   : reply-code
      Response that represents to specify SMTP response code.
   : add-header
      Response that represents to append header.
   : insert-header
      Response that represents to insert header into any position.
   : change-header
      Response that represents to change header.
   : add-recipient
      Response that represents to change recipient.
   : delete-recipient
      Response that represents to delte recipient.
   : replace-body
      Response that represents to replace body.
   : progress
      Response that represents to be in progress.
      This response is used to increase the timeout.
   : quarantine
      Response that represents to quarantine the mail.
   : skip
      Response that represents to skip receiving body.

: error: [server][timeout][connection]

   Connection timeout.

: error: [server][error][connect]

   Connection error.

: error: [server][error][connected][start]

   Error occurs while initializing to start connecting.

: warning: [server][reply][quitted][#{state}][#{response}]

   Server receives a response after finished milter protocol.
   "state" and "response" are same as "[server][error][#{response}][state][invalid][#{state}]".
   No problem if there are no error log after this log.

=== manager

This is log list of manager module.

There is no "session_id" in log not related to session.

==== submodule

First tag is submodule name in manager module. However, there is no
tag when milter-manager crashes in manager module and when
milter-manager is booting.

Submodules are 8 items as following:

  * manager
  * configuration
  * launcher
  * process-launcher
  * controller
  * leader
  * children
  * egg

: manager
   This submodule implements milter-manager command using each submodules.

: configuration
   This submodule loads configuration file.

: launcher
   This submodule launches child-milter.

: process-launcher
   This submodule launches launcher submodule in milter-manager command.

: controller
   This submodule operates milter-manager process from the outside.

: leader
   This submodule returns result that children module collect all
   child-milters' result to MTA.

: children
   This submodule collect and summarize multiple child-milters' result.

: egg
   This module manages information of one child-milter.

Describe for each submodule.

==== submodule: manager

: error: [manager][reload][signal][error]

   Error occurs while reloading configuration requested by SIGHUP signal.

==== submodule: configuration

: error: [configuration][dispose][clear][error]

   Fail to clear configuration while disposing.

: error: [configuration][new][clear][error]

   Fail to clear configuration at the first time.

: error: [configuration][load][clear][error]

   Fail to clear configuration before reload configuration.

: error: [configuration][load][error]

   Fail to load milter-manager.conf.

: error: [configuration][load][custom][error]

   Fail to load milter-manager.custom.conf.

: error: [configuration][clear][custom][error]

   Fail to clear configuration using Ruby.

: error: [configuration][maintain][error]

   Error occurs while processing of maintenance every maitenance_interval seconds.
   See ((<manager.maintenance_interval|configuration.rd#manager.maintenance_interval>)).

: error: [configuration][event-loop-created][error]

   Error occurs after create event loop.

==== submodule: launcher

: error: [launcher][error][child][authority][group]

   Fail to change execution privilege to specified group when launch
   child-milter.

: error: [launcher][error][child][authority][groups]

   Fail to initialize specified additional group when launch child-milter.

: error: [launcher][error][child][authority][user]

   Fail to change execution privilege to specified user when launch child-milter.

: error: [launcher][error][launch]

   Fail to launch child-milter.

: error: [launcher][error][write]

   Error occurs when this module writes response whether success or not.

==== submodule: process-launcher

: error: [process-launcher][error][start]

   Error occurs when this module start connecting with launcher module.

: error: [process-launcher][error]

   Fail to detach file descriptors from forked launcher module.

==== submodule: controller

: error: [controller][error][write][success]

   Error occurs when this module writes successful response.

: error: [controller][error][write][error]

   Error occurs when this module writes error response.

: error: [controller][error][save]

   Error occurs when this module saves configuration file.

: error: [controller][error][write][configuration]

   Error occurs when this module writes configuration file.

: error: [controller][reload][error]

   Error occurs when this module reloads configuration.

: error: [controller][error][write][status]

   Error occurs when this module writes status response.

: error: [controller][error][unix]

   Error occurs when this module deletes UNIX domain socket if enabled remove_unix_socket_on_create.
   See ((<controller.remove_unix_socket_on_create|configuration.rd#controller.remove_unix_socket_on_create>)).

: error: [controller][error][start]

   Fail to start communication.

: error: [controller][error][accept]

   Fail to accept connection.

: error: [controller][error][watch]

   Error occurs while communicating.

: error: [controller][error][listen]

   Fail to call listen(2).

==== submodule: leader

: error: [leader][error][invalid-state]

   Detect invalid state transition. Probably this is milter manager's bug.

: error: [leader][error]

   Error occurs in children module.

: error: [leader][error][reply-code]

   Fail to specify SMTP response code. Milter may send invalid response code.

: error: [leader][error][add-header]

   Error occurs when append header.

: error: [leader][error][insert-header]

   Error occurs when insert header into any position.

: error: [leader][error][delete-header]

   Error occurs when delete header.

: error: [leader][error][change-from]

   Error occurs when change recipient.

: error: [leader][error][add-recipient]

   Error occurs when add recipient.

: error: [leader][error][delete-recipient]

   Error occurs when delete recipient.

: error: [leader][error][replace-body]

   Error occurs when replace body.

==== submodule: children

: error: [children][error][negotiate]

   This module receives response though this module does not send
   request to start negotiation. Milter may violate milter protocol.

: error: [children][error][pending-message-request]

   A request to child-milter has a problem when milter-manager process
   delays it. If you find this log, milter manager has a bug.

: error: [children][error][body][read][seek]

   Error occurs when seek position to read body.

: error: [children][error][body][read]

   Error occurs while reading body.

: error: [children][error][invalid-state][temporary-failure]

   Milter respond "temporary-failure" response at unexpected timing in
   milter protocol. Milter may violate milter protocol.

: error: [children][error][invalid-state][reject]

   Milter respond "reject" response at unexpected timing in
   milter protocol. Milter may violate milter protocol.

: error: [children][error][invalid-state][accept]

   Milter respond "accept" response at unexpected timing in
   milter protocol. Milter may violate milter protocol.

: error: [children][error][invalid-state][discard]

   Milter respond "discard" response at unexpected timing in
   milter protocol. Milter may violate milter protocol.

: error: [children][error][invalid-state][stopped]

   milter-manager process stops child-milter at unexpected timing in
   milter protocol. Probably this is milter manager's bug.

: error: [children][error][invalid-state][#{response}][#{state}]

   Milter can return "#{response}" at state of "end-of-message". However milter
   returns "#{response}" at state of "#{state}".
   Milter may violate milter protocol.

   "#{response}" is as following:

   : add-header
      Response represents to append header.
   : insert-header
      Response represents to insert header into any position.
   : change-header
      Response represents to change header.
   : delete-header
      Response represents to delete header.
   : change-from
      Response represents to change from.
   : add-recipient
      Response represents to add recipient.
   : delete-recipient
      Response represents to delete recipient.
   : replace-body
      Response represents to replace body.
   : progress
      Response represents process is in progress.
   : quarantine
      Response represents to quarantine a mail.

   "#{state}" is same as "[server][error][#{response}][state][invalid][#{state}]".

: error: [children][timeout][writing]

   Writing timeout.

: error: [children][timeout][reading]

   Reading timeout.

: error: [children][timeout][end-of-message]

   Response timeout when "end-of-message".

: error: [children][error][#{state}][#{fallback_status}]

   Error occurs in server module that is used communication with child-milter.
   See ((<milter.fallback_status|configuration.rd#milter.fallback_status>)) about child-milter result.
   See "[server][error][#{response}][state][invalid][#{state}]" abount "#{state}".

: error: [children][error][start-child][write]

   Error occurs when this module requests launcer submodule to lauch
   child-milter.

: error: [children][error][start-child][flush]

   Error occurs when this module flashes request that starts
   child-milter to launcher submodule.

: error: [children][error][negotiate][not-started]

   This module executes process of negotiation response but MTA does
   not start negotiation. Probably milter manager has a bug if you
   find this log.

: error: [children][error][negotiate][no-response]

   All milters do not respond to negotiation. Probably child-milter
   has some problems if you find this log.

: error: [children][timeout][connection]

   Connection timeout.

: error: [children][error][connection]

   Error occurs while processing connection.

: error: [children][error][alive]

   MTA send requests to this module though all child-milter have
   finished.

: error: [children][error][message-processing]

   MTA send request that process mail to this module though no
   child-milter processes data which is sent via SMTP DATA.

: error: [children][error][body][open]

   Fail to create temporary file to save large mail body.

: error: [children][error][body][encoding]

   Fail to set encoding to read/write temporary file to save large
   mail body.

: error: [children][error][body][write]

   Fail to write data to temporary file to save large mail body.

: error: [children][error][body][send][seek]

   Fail to seek position in temporary file to save large mail body.

: error: [children][error][body][send]

   Fail to read data from temporary file to save large mail body.

==== submodule: egg

: error: [egg][error]

   Detect invalid connection spec is specified when this module
   connect to child-milter.

: error: [egg][error][set-spec]

   Set invalid connection spec to child-milter.

==== other: logs when crashed

: critical: [#{signal}] unable to open pipe for collecting stack trace

   Unable to open pipe for collecting stack trace.
   Signal is SEGV or ABORT.

: critical: #{stack_trace}

   No tags. Display stack trace for each line.

==== other: starting milter-manager command

: error: failed to create pipe for launcher command

   Failed to create pipe for launcher command.

: error: failed to create pipe for launcher reply

   Failed to create pipe for launcher reply.

: error: failed to fork process launcher process

   Failed to fork process launcher process.

: error: failed to find password entry for effective user

   Failed to find password entry for effective user.

: error: failed to get password entry for effective user

   Failed to get password entry for effective user.

: error: failed to get limit for RLIMIT_NOFILE

   Failed to get limit for RLIMIT_NOFILE.

: error: failed to set limit for RLIMIT_NOFILE

   Failed to set limit for RLIMIT_NOFILE.

: error: failed to create custom configuration directory

   Failed to create custom configuration directory.

: error: failed to change owner and group of configuration directory

   Failed to change owner and group of configuration directory

: error: failed to listen

   Failed to listen(2).

: error: failed to drop privilege

   Failed to drop root privilege.

: error: failed to listen controller socket:

   Failed to listen(2) controller socket.

: error: failed to daemonize:

   Failed to daemonize.

: error: failed to start milter-manager process:

   Failed to start milter-manager process.

: error: [manager][reload][custom-load-path][error]

   Failed to reload configuration.

: error: [manager][configuration][reload][command-line-load-path][error]

   Failed to load configuration from load path specified by command line.


==== other: statistics

: statistics: [milter][header][add]

   Child-milter adds header.

: statistics: [milter][end][#{last_state}][#{status}][#{elapsed}]

   One child-milter has finished.

   "[#{last_state}]" represents state that it has finished.
   See "#{state}" of "[server][error][#{response}][state][invalid][#{state}]".

   "#{status}" is response from child-milter. Response is as following.

   : reject
      Response represents to reject.
   : discard
      Response represents to discard.
   : accept
      Response represents to accept.
   : temporary-failure
      Response represents temporary failure.
   : pass
      Response represents to pass implicitly.
   : stop
      Response represents to stop process in progress..

   "[#{elapsed}]" represents elapsed time since connection start.

: statistics: [session][end][#{last_state}][#{status}][#{elapsed}]

   One milter session has finished.
   See "[milter][end][#{last_state}][#{status}][#{elapsed}]".

: statistics: [session][disconnected][#{elapsed}]

   SMTP client has disconnected the session in progress.

   "[#{elapsed}]" represents elapsed time since connection start.

: statistics: [session][header][add]

   Add header as response of whole milter session.
