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

