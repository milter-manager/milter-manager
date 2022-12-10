---
title: milter-statistics-report / milter manager / milter manager's manual
---

# milter-statistics-report / milter manager / milter manager's manual

## NAME

milter-statistics-report - milter process statistics reporter

## SYNOPSIS

<code>milter-statistics-report</code> [<em>option ...</em>] <em>target process name ...</em>

## DESCRIPTION

milter-statistics-report observes milter processes andreports statistics of them periodically. Here are reportedstatistics.

* PID (Process ID)
* VSS (Virtual Set Size)
* RSS (Resident Set Size)
* CPU usage percent
* Used CPU time
* Number of opened file descriptors

milter-report-statistics can handle any process even if itisn't milter.

## OPTIONS

<dl>
<dt>--help</dt>
<dd>Shows available options and exits.</dd>
<dt>--filter=REGEXP</dt>
<dd>Filters target processes by regular expression<var>REGEXP</var> . Processes which matches their command linein all processes that has <em>target process name</em> to<var>REGEXP</var> are only reported their statistics.

The default is none. (No filter is applied.)</dd>
<dt>--interval=INTERVAL</dt>
<dd>Reports statistics at intervals of INTERVALseconds/minutes/hours.

The default is 1 second.</dd></dl>

## EXIT STATUS

Always 0.

## EXAMPLE

In the following example, milter-test-client and smtpd areobserved:

<pre>% milter-report-statistics milter-test-client smtpd
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
16:42:08  23242  56656 kB   2904 kB   4.0  0:00.03     0 smtpd -n smtp -t inet -u</pre>

In the following example, milter implemented by Ruby areobserved at intervals of 1 second:

<pre>% milter-report-statistics --filter milter --interval 5 ruby
    Time    PID       VSS       RSS  %CPU CPU time   #FD command
16:44:45  23257 184224 kB   9700 kB   0.0  0:05.79    10 ruby milter-test-client.rb
16:44:50  23257 184224 kB   9700 kB  34.0  0:07.02    14 ruby milter-test-client.rb
16:44:55  23257 184224 kB   9700 kB  36.0  0:08.79    13 ruby milter-test-client.rb
16:45:00  23257 184224 kB   9728 kB  36.0  0:10.57    14 ruby milter-test-client.rb
16:45:05  23257 184224 kB   9728 kB  36.0  0:11.42    14 ruby milter-test-client.rb</pre>

## SEE ALSO

milter-performance-check.rd.ja(1)


