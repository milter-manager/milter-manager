---
title: milter-performance-check / milter manager / milter manager's manual
---

# milter-performance-check / milter manager / milter manager's manual

## NAME

milter-performance-check - MTA performance check program

## SYNOPSIS

<code>milter-performance-check</code> [<em>option ...</em>]

## DESCRIPTION

milter-performance-check is a SMTP client that measures MTAperformance. milter-test-server measures milter'sperformance and miter-performance-check measures MTA +milter's performance.

smtp-source bundled with Postfix is a similartool. smtp-source has more features.

milter-performance-check is useful because it measureselapsed time of only SMTP sessions. smtp-source doesn'tprovide the feature. You need to also use 'time' command andmeasure smtp-source command runtime not elapsed time of SMTPsessions.

Normally, there isn't big difference between elapsed time ofSMTP sessions and tool runtime.

If you satisfy milter-performance-check's features, it'sbetter that you use milter-performance-check. If you don't,smtp-source is more better tool for you.

## Options

<dl>
<dt>--help</dt>
<dd>Shows available options and exits.</dd>
<dt>--smtp-server=SERVER</dt>
<dd>Uses SEVER as target SMTP server.

The default is localhost.</dd>
<dt>--smtp-port=PORT</dt>
<dd>Uses PORT as SMTP port.

The default is 25.</dd>
<dt>--connect-host=HOST</dt>
<dd>Specifies connected SMTP client host.

This uses NAME of [Postfix's XCLIENT SMTPextension](http://www.postfix.com/XCLIENT_README.html).[smtpd_authorized_xclient_hosts](http://www.postfix.org/postconf.5.html#smtpd_authorized_xclient_hosts)should be configured appropriately.</dd>
<dt>--connect-address=ADDRESS</dt>
<dd>Specifies connected SMTP client address.

This uses ADDR of [Postfix's XCLIENT SMTPextension](http://www.postfix.com/XCLIENT_README.html).[smtpd_authorized_xclient_hosts](http://www.postfix.org/postconf.5.html#smtpd_authorized_xclient_hosts)should be configured appropriately.</dd>
<dt>--helo-fqdn=FQDN</dt>
<dd>Uses FQDN as HELO SMTP command.

The default localhost.localdomain.</dd>
<dt>--starttls=HOW</dt>
<dd>Since 1.6.9.

Specifies whether STARTTLS is used or not. Here areavailable <var>HOW</var> values:

<dl>
<dt><kbd>auto</kbd></dt>
<dd>It uses STARTTLS when MTA supports STARTTLS. (default)</dd>
<dt><kbd>always</kbd></dt>
<dd>It always uses STARTTLS.</dd>
<dt><kbd>disable</kbd></dt>
<dd>It never use STARTTLS.</dd></dl>

The default is <kbd>auto</kbd>.</dd>
<dt>--auth-user=USER</dt>
<dd>Since 1.6.9.

Uses <var>USER</var> as SMTP Authentication user.

The default is none.</dd>
<dt>--auth-password=PASSWORD</dt>
<dd>Since 1.6.9.

Uses <var>PASSWORD</var> as SMTP Authentication password.

The default is none.</dd>
<dt>--auth-mechanism=MECHANISM</dt>
<dd>Since 1.6.9.

Uses <var>MECHANISM</var> as SMTP Authenticationmechanism. Here are available <var>MECHANISM</var> values:

<dl>
<dt><kbd>auto</kbd></dt>
<dd>It uses a detected available mechanism by MTA.(default)</dd>
<dt><kbd>plain</kbd></dt>
<dd>It always uses PLAIN.</dd>
<dt><kbd>login</kbd></dt>
<dd>It always uses LOGIN.</dd>
<dt><kbd>cram_md5</kbd> or <kbd>cram-md5</kbd></dt>
<dd>It always uses CRAM-MD5.</dd></dl>

The default is <kbd>auto</kbd>.</dd>
<dt>--auth-map=FILE</dt>
<dd>Since 1.6.9.

Loads SMTP Authentication configurations for MTA addressand port number pairs from <var>FILE</var>.

Here is <var>FILE</var> format that is also used by [Postfix'ssmtp_sasl_password_maps](http://www.postfix.org/postconf.5.html#smtp_sasl_password_maps):

<pre>SERVER1:PORT USER1:PASSWORD1
SERVER2:PORT USER2:PASSWORD2
...</pre>

Here is an example configuration that "send-user" user and"secret" password account is used for "smtp.example.com"address and "submission port" (587 port) MTA:

<pre>smtp.example.com:587 send-user:secret</pre>

The default is none.</dd>
<dt>--from=FROM</dt>
<dd>Uses FROM as MAIL SMTP command.

The default from@example.com.</dd>
<dt>--force-from=FROM</dt>
<dd>Uses FROM as MAIL SMTP command even if mail file includes"From:" header.

The default is none.</dd>
<dt>--recipient=RECIPIENT</dt>
<dd>Uses RECIPIENT as RCPT SMTP command. If you want to usemultiple recipients, use --recipient n-times.

The default is [to@example.com].</dd>
<dt>--force-recipient=RECIPIENT</dt>
<dd>Uses RECIPIENT as RCPT SMTP command even if mail fileincludes 'To:' header. If you want to use multiplerecipients, use --recipient n-times.

The default is none.</dd>
<dt>--n-mails=N</dt>
<dd>Sends N mails. Some mails are sent concurrently.Maximum concurrency can be specified by--n-concurrent-connections.

The default is 100.</dd>
<dt>--n-additional-lines=N</dt>
<dd>Adds N lines into mail body.

The default is none. (Doesn't add.)</dd>
<dt>--n-concurrent-connections=N</dt>
<dd>Sends mails with N connections concurrently.

The default is 10.</dd>
<dt>--period=PERIOD</dt>
<dd>Sends mails in PERIOD seconds/minutes/hours. Each mail issent averagely. PERIOD is treated as seconds when itsunit is omitted.

Example (100 mails are sent):

* --period=5    # sends mails at intervals of 0.05 seconds (5 / 100)
* --period=50s  # sends mails at intervals of 0.5 seconds (50 / 100)
* --period=10m  # sends mails at intervals of 6 seconds (60 * 10 / 100)
* --period=0.5h # sends mails at intervals of 18 seconds (60 * 60 * 0.5 / 100)

The default is none.</dd>
<dt>--interval=INTERVAL</dt>
<dd>Sends mails at intervals of INTERVAL seconds/minutes/hours.INTERVAL is treated as seconds when its unit is omitted.

Example:

* --interval=5    # sends mails at intervals of 5 seconds
* --interval=0.5s # sends mails at intervals of 0.5 seconds
* --interval=10m  # sends mails at intervals of 10 minutes
* --interval=0.5h # sends mails at intervals of 0.5 hours

The default is none.</dd>
<dt>--flood[=PERIOD]</dt>
<dd>Sends flood of mails in PERIOD seconds/minutes/hours. IfPERIOD is omitted, floods mails endlessly. PERIOD istreated as seconds when its unit is omitted.

The default is none.</dd>
<dt>--shuffle, --no-shuffle</dt>
<dd>Shuffles target mails before sending.

The default is false. (don't shuffle.)</dd>
<dt>--report-failure-responses, --no-report-failure-responses</dt>
<dd>Reports failure messages from SMTP server at the last.

The default is false. (don't report.)</dd>
<dt>--report-periodically[=INTERVAL]</dt>
<dd>Reports statistics at intervals of INTERVALseconds/minutes/hours. If INTERVAL is omitted, 1s (1second) is used. INTERVAL is treated as seconds when itsunit is omitted.

The default is one. (Don't report statistics periodically.)</dd>
<dt>--reading-timeout=SECONDS</dt>
<dd>Specifies timeout on reading a response from a SMTP server.An error is occurred when the SMTP server doesn'trespond to a request in <var>SECONDS</var> seconds.

The default is 60 seconds.</dd></dl>

## EXIT STATUS

Always 0.

## EXAMPLE

In the following example, milter-performance-check connectsa SMTP server running on localhost at 25 port and sends 100mails. Each mail's sender is from@example.com andrecipients are webmaster@localhost and info@localhost.

<pre>% milter-performance-check --recipient=webmaster@localhost --recipient=info@localhost</pre>

In the following example, milter-performance-check connectsa SMTP server running on 192.168.1.102 at 25 port and sendsfiles under /tmp/test-mails/ directory. The files should beRFC 2822 format. The mails are sent to user@localhost atintervals of 3 seconds (60 * 10 / 100). Each mail is sentonly 1 time because of --n-mails=1 option.

<pre>% milter-performance-check --n-mails=1 --smtp-server=192.168.1.102 --force-recipient=user@localhost --period=5m /tmp/test-mails/</pre>

## SEE ALSO

milter-performance-check.rd.ja(1)


