= milter-performance-check / milter manager / milter manager's manual

== NAME

milter-performance-check - MTA performance check program

== SYNOPSIS

(({milter-performance-check})) [((*option ...*))]

== DESCRIPTION

milter-performance-check is a SMTP client that measures MTA
performance. milter-test-server measures milter's
performance and miter-performance-check measures MTA +
milter's performance.

smtp-source bundled with Postfix is a similar
tool. smtp-source has more features.

milter-performance-check is useful because it measures
elapsed time of only SMTP sessions. smtp-source doesn't
provide the feature. You need to also use 'time' command and
measure smtp-source command runtime not elapsed time of SMTP
sessions.

Normally, there isn't big difference between elapsed time of
SMTP sessions and tool runtime.

If you satisfy milter-performance-check's features, it's
better that you use milter-performance-check. If you don't,
smtp-source is more better tool for you.

== Options

: --help

   Shows available options and exits.

: --smtp-server=SERVER

   Uses SEVER as target SMTP server.

   The default is localhost.

: --smtp-port=PORT

   Uses PORT as SMTP port.

   The default is 25.

: --connect-host=HOST

   Specifies connected SMTP client host.

   This uses NAME of ((<Postfix's XCLIENT SMTP
   extension|URL:http://www.postfix.com/XCLIENT_README.html>)).
   ((<smtpd_authorized_xclient_hosts|URL:http://www.postfix.org/postconf.5.html#smtpd_authorized_xclient_hosts>))
   should be configured appropriately.

: --connect-address=ADDRESS

   Specifies connected SMTP client address.

   This uses ADDR of ((<Postfix's XCLIENT SMTP
   extension|URL:http://www.postfix.com/XCLIENT_README.html>)).
   ((<smtpd_authorized_xclient_hosts|URL:http://www.postfix.org/postconf.5.html#smtpd_authorized_xclient_hosts>))
   should be configured appropriately.

: --helo-fqdn=FQDN

   Uses FQDN as HELO SMTP command.

   The default localhost.localdomain.

: --from=FROM

   Uses FROM as MAIL SMTP command.

   The default from@example.com.

: --force-from=FROM

   Uses FROM as MAIL SMTP command even if mail file includes
   "From:" header.

   The default is none.

: --recipient=RECIPIENT

   Uses RECIPIENT as RCPT SMTP command. If you want to use
   multiple recipients, use --recipient n-times.

   The default is [to@example.com].

: --force-recipient=RECIPIENT

   Uses RECIPIENT as RCPT SMTP command even if mail file
   includes 'To:' header. If you want to use multiple
   recipients, use --recipient n-times.

   The default is none.

: --n-mails=N

   Sends N mails. Some mails are sent concurrently.
   Maximum concurrency can be specified by
   ((<--n-concurrent-connections|.#--n-concurrent-connections>)).

   The default is 100.

: --n-concurrent-connections=N

   Sends mails with N connections concurrently.

   The default is 10.

: --period=PERIOD

   Sends mails in PERIOD seconds/minutes/hours. Each mail is
   sent averagely. PERIOD is treated as seconds when its
   unit is omitted.

   Example (100 mails are sent):
     * --period=5    # sends mails at intervals of 0.05 seconds (5 / 100)
     * --period=50s  # sends mails at intervals of 0.5 seconds (50 / 100)
     * --period=10m  # sends mails at intervals of 6 seconds (60 * 10 / 100)
     * --period=0.5h # sends mails at intervals of 18 seconds (60 * 60 * 0.5 / 100)

   The default is none.

: --interval=INTERVAL

   Sends mails at intervals of INTERVAL seconds/minutes/hours.
   INTERVAL is treated as seconds when its unit is omitted.

   Example:
     * --interval=5    # sends mails at intervals of 5 seconds
     * --interval=0.5s # sends mails at intervals of 0.5 seconds
     * --interval=10m  # sends mails at intervals of 10 minutes
     * --interval=0.5h # sends mails at intervals of 0.5 hours

   The default is none.

: --shuffle, --no-shuffle

   Shuffles target mails before sending.

   The default is false. (don't shuffle.)

: --reading-timeout=SECONDS

    Specifies timeout on reading a response from a SMTP server.
    An error is occurred when the SMTP server doesn't
    respond to a request in ((|SECONDS|)) seconds.

   The default is 60 seconds.

== EXIT STATUS

Always 0.

== EXAMPLE

In the following example, milter-performance-check connects
a SMTP server running on localhost at 25 port and sends 100
mails. Each mail's sender is from@example.com and
recipients are webmaster@localhost and info@localhost.

  % milter-performance-check --recipient=webmaster@localhost --recipient=info@localhost

In the following example, milter-performance-check connects
a SMTP server running on 192.168.1.102 at 25 port and sends
files under /tmp/test-mails/ directory. The files should be
RFC 2822 format. The mails are sent to user@localhost at
intervals of 3 seconds (60 * 10 / 100). Each mail is sent
only 1 time because of --n-mails=1 option.

  % milter-performance-check --n-mails=1 --smtp-server=192.168.1.102 --force-recipient=user@localhost --period=5m /tmp/test-mails/

== SEE ALSO

((<milter-test-client.rd>))(1),
((<milter-test-server.rd>))(1)
