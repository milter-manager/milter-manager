= milter-statistics-report / milter manager / milter manager's manual

== NAME

milter-statistics-report - milter process statistics reporter

== SYNOPSIS

(({milter-statistics-report})) [((*option ...*))] ((*target process name ...*))

== DESCRIPTION

milter-statistics-report observes milter processes and
reports statistics of them periodically. Here are reported
statistics.

  * PID (Process ID)
  * VSS (Virtual Set Size)
  * RSS (Resident Set Size)
  * CPU usage percent
  * Used CPU time
  * Number of opened file descriptors

milter-report-statistics can handle any process even if it
isn't milter.

== OPTIONS

: --help

   Shows available options and exits.

: --filter=REGEXP

   Filters target processes by regular expression
   ((|REGEXP|)) . Processes which matches their command line
   in all processes that has ((*target process name*)) to
   ((|REGEXP|)) are only reported their statistics.

   The default is none. (No filter is applied.)

: --interval=INTERVAL

   Reports statistics at intervals of INTERVAL
   seconds/minutes/hours.

   The default is 1 second.

== EXIT STATUS

Always 0.

== EXAMPLE

In the following example, milter-test-client and smtpd are
observed:

  % milter-report-statistics milter-test-client smtpd
      Time    PID       VSS       RSS  %CPU CPU time   #FD command
  16:42:02  23133  37060 kB   1652 kB   0.0  0:00.00    10 milter-test-client -s inet:10025
  16:42:02  23231  56656 kB   2904 kB   4.0  0:00.06     0 smtpd -n smtp -t inet -u
  16:42:03  23133  37060 kB   1652 kB   0.0  0:00.00    10 milter-test-client -s inet:10025
  16:42:03  23234  56656 kB   2900 kB   2.0  0:00.02     0 smtpd -n smtp -t inet -u
  16:42:04  23133  37060 kB   1652 kB   0.0  0:00.00    10 milter-test-client -s inet:10025
  16:42:05  23133  37060 kB   1652 kB   0.0  0:00.00    10 milter-test-client -s inet:10025
  16:42:05  23237  56656 kB   2904 kB   4.0  0:00.04     0 smtpd -n smtp -t inet -u
  16:42:06  23133  37060 kB   1652 kB   0.0  0:00.00    10 milter-test-client -s inet:10025
  16:42:06  23239  57436 kB   2900 kB   4.0  0:00.02     0 smtpd -n smtp -t inet -u
  16:42:07  23133  37060 kB   1652 kB   0.0  0:00.00    10 milter-test-client -s inet:10025
  16:42:07  23239  57436 kB   2900 kB                    0 smtpd -n smtp -t inet -u
  16:42:08  23133  37060 kB   1652 kB   0.0  0:00.00    10 milter-test-client -s inet:10025
  16:42:08  23242  56656 kB   2904 kB   4.0  0:00.03     0 smtpd -n smtp -t inet -u

In the following example, milter implemented by Ruby are
observed at intervals of 1 second:

  % milter-report-statistics --filter milter --interval 5 ruby
      Time    PID       VSS       RSS  %CPU CPU time   #FD command
  16:44:45  23257 184224 kB   9700 kB   0.0  0:05.79    10 ruby milter-test-client.rb
  16:44:50  23257 184224 kB   9700 kB  34.0  0:07.02    14 ruby milter-test-client.rb
  16:44:55  23257 184224 kB   9700 kB  36.0  0:08.79    13 ruby milter-test-client.rb
  16:45:00  23257 184224 kB   9728 kB  36.0  0:10.57    14 ruby milter-test-client.rb
  16:45:05  23257 184224 kB   9728 kB  36.0  0:11.42    14 ruby milter-test-client.rb

== SEE ALSO

((<milter-performance-check.rd.ja>))(1)
