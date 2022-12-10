---
title: Install to FreeBSD
---

# Install to FreeBSD --- How to install milter manager to FreeBSD

## About this document

This document describes how to install milter manager toFreeBSD. See Install for general installinformation.

This document assumes that FreeBSD 10.0-RELEASE is used.

## Install packages

To install the following packages, related packages are alsoinstalled:

We use Postfix as MTA:

<pre>% sudo pkg install --yes postfix</pre>

We use spamass-milter, clamav-milter and milter-greylist asmilters:

<pre>% sudo pkg install --yes spamass-milter milter-greylist clamav-milter</pre>

Install milter-manager.

<pre>% sudo pkg install --yes milter-manager</pre>

## Configuration

Here is a basic configuration policy.

We use UNIX domain socket for accepting connection fromMTA because security and speed.

We use general user for milter's effective user. This isalso for security. 'mail' group has permission of read/writeUNIX domain socket. 'postfix' user is joined to 'mail' group.

milter-greylist should be applied only if[S25R](http://gabacho.reto.jp/en/anti-spam/)condition is matched to reduce needless delivery delay.But the configuration is automatically done bymilter-manager. We need to do nothing for it.

### Configure spamass-milter

First, we configure spamd.

We create /usr/local/etc/mail/spamassassin/local.cf with thefollowing configuration. This configuration is for addingheaders only if spam detected.

<pre>remove_header ham Status
remove_header ham Level</pre>

We need to append the following to /etc/rc.conf to enablespamd:

<pre>spamd_enable=YES</pre>

If our SMTP server has many concurrent connections, weshould increase max concurrent connections. It is 5 bydefault. It's a good first value that about 1/3 of the maxSMTP connections. e.g. about 30 for about 100 connectionsSMTP server:

<pre>spamd_flags="-c --max-children=30 "</pre>

We can adjust apposite value after operation. We can seemilter manager's statistics graphs at the time.

Update Spamassassin's rule file and start spamd:

<pre>% sudo sa-update
% sudo /usr/sbin/service sa-spamd start</pre>

Next, we configure spamass-milter. We run spamass-milterwith 'spamd' user and 'spamd' group.

spamass-milter creates a socket fileas /var/run/spamass-milter.sock by default. But a general usercan't create a new file in /var/run/. We create/var/run/spamass-milter/ directory owned by 'spamd'user. spamass-milter creates a socket file in the directory:

<pre>% sudo mkdir /var/run/spamass-milter/
% sudo /usr/sbin/chown spamd:spamd /var/run/spamass-milter</pre>

We add the following to /etc/rc.conf:

<pre>spamass_milter_enable="YES"
spamass_milter_user="spamd"
spamass_milter_group="spamd"
spamass_milter_socket="/var/run/spamass-milter/spamass-milter.sock"
spamass_milter_socket_owner="spamd"
spamass_milter_socket_group="mail"
spamass_milter_socket_mode="660"
spamass_milter_localflags="-u spamd -- -u spamd"</pre>

spamass-milter should be started:

<pre>% sudo /usr/sbin/service spamass-milter start</pre>

### Configure clamav-milter

First, we configure ClamAV.

We add the following to /etc/rc.conf to enable clamd andfreshclam:

<pre>clamav_clamd_enable="YES"
clamav_freshclam_enable="YES"</pre>

Get the latest definition files before run clamd:

<pre>% sudo /usr/local/bin/freshclam</pre>

clamd and freshclam should be started:

<pre>% sudo /usr/sbin/service clamav-clamd start
% sudo /usr/sbin/service clamav-freshclam start</pre>

clamav-milter is ran as 'clamav' user and 'clamav' group bydefault. We use the configuration because 'clamav' user isgeneral user. We set group read/write permission of socket.

We add the following to /etc/rc.conf:

<pre>clamav_milter_enable="YES"
clamav_milter_socket_mode="660"
clamav_milter_socket_group="mail"</pre>

We may need to configure /usr/local/etc/clamav-milter.conf.e.g.:

/usr/local/etc/clamav-milter.conf

Before:

<pre>#OnInfected Quarantine
#AddHeader Replace
#LogSyslog yes
#LogFacility LOG_MAIL
#LogInfected Basic</pre>

After:

<pre>OnInfected Reject
AddHeader Replace
LogSyslog yes
LogFacility LOG_MAIL
LogInfected Full</pre>

Here are explanations of the above configurations:

<dl>
<dt>OnInfected Reject</dt>
<dd>Rejects infected mails. The default value isQuarantine. It puts infected mails into Postfix's holdqueue. If we don't want to confirm hold queueperiodically, Reject is a good way for easy maintenance.</dd>
<dt>AddHeader Replace</dt>
<dd>Replaces X-Virus-Scanned header even if it's existed.</dd>
<dt>LogSyslog yes</dt>
<dd>Logs to syslog.</dd>
<dt>LogFacility LOG_MAIL</dt>
<dd>Logs to syslog with LOG_MAIL facility. /var/log/maillogis the default LOG_MAIL log file.</dd>
<dt>LogInfected Full</dt>
<dd>Logs verbosity on finding infected mails.</dd></dl>

clamav-milter should be started:

<pre>% sudo /usr/sbin/service clamav-milter start</pre>

### Configure milter-greylist

We run milter-greylist as 'mailnull' user and 'mailnull' group.'mailnull' user is the default configuration and it is unuseduser on Postfix environment.

We copy /usr/local/etc/mail/greylist.conf.sample to/usr/local/etc/mail/greylist.conf and change it for thefollowing configurations:

* change the effective group to "mail" group from "mailnull" group.
* add write permission for the socket file to "mail" group.
* use the leading 24bits for IP address match to avoid    Greylist adverse effect for sender uses some MTA case.
* decrease retransmit check time to 10 minutes from 30    minutes (default value) to avoid Greylist adverse effect.
* increase auto whitelist period to a week from 1 day    (default value) to avoid Greylist adverse effect.
* use Greylist by default.

<pre># note
The configuration relaxes Greylist check to avoid Greylist
adverse effect. It increases received spam mails but we
should give priority to avoid false positive rather than
false negative. We should not consider that we blocks all
spam mails by Greylist. We can blocks spam mails that
isn't blocked by Greylist by other anti-spam technique
such as SpamAssassin. milter manager helps constructing
mail system that combines some anti-spam techniques.</pre>

Before:

<pre>socket "/var/milter-greylist/milter-greylist.sock"
user "mailnull:mailnull"
racl whitelist default</pre>

After:

<pre>socket "/var/milter-greylist/milter-greylist.sock" 660
user "mailnull:mail"
subnetmatch /24
greylist 10m
autowhite 1w
racl greylist default</pre>

We add the following to /etc/rc.conf:

<pre>miltergreylist_enable="YES"
miltergreylist_runas="mailnull:mail"</pre>

milter-greylist should be started:

<pre>% sudo /usr/sbin/service milter-greylist start</pre>

### Configure milter-manager

We create 'milter-manager' user because we runmilter-manager as 'milter-manager' user:

<pre>% sudo /usr/sbin/pw groupadd milter-manager
% sudo /usr/sbin/pw useradd milter-manager -g milter-manager -G mail -m</pre>

milter-manager detects milters that installed in system.We can confirm spamass-milter, clamav-milter andmilter-greylist are detected:

<pre>% sudo /usr/local/sbin/milter-manager -u milter-manager --show-config</pre>

The following output shows milters are detected:

<pre>...
define_milter("milter-greylist") do |milter|
  milter.connection_spec = "unix:/var/milter-greylist/milter-greylist.sock"
  ...
  milter.enabled = true
  ...
end
..
define_milter("clamav-milter") do |milter|
  milter.connection_spec = "unix:/var/run/clamav/clmilter.sock"
  ...
  milter.enabled = true
  ...
end
..
define_milter("spamass-milter") do |milter|
  milter.connection_spec = "unix:/var/run/spamass-milter/spamass-milter.sock"
  ...
  milter.enabled = true
  ...
end
..</pre>

We should confirm that milter's name, socket path and'enabled = true'. If the values are unexpected,we need to change/usr/local/etc/milter-manager/milter-manager.conf.See Configuration for details ofmilter-manager.conf.

But if we can, we want to use milter manager without editingmiter-manager.conf. If you report your environment to themilter manager project, the milter manager project mayimprove detect method.

milter-manager creates socket file as/var/run/milter-manager/milter-manager.sock by default onFreeBSD. We need to create /var/run/milter-manager directorybefore running milter-manager:

<pre>% sudo mkdir -p /var/run/milter-manager
% sudo /usr/sbin/chown -R milter-manager:milter-manager /var/run/milter-manager</pre>

milter-manager's configuration is completed. We start tosetup running milter-manager.

We add the following to /etc/rc.conf to enable milter-manager:

<pre>miltermanager_enable="YES"</pre>

milter-manager should be started:

<pre>% sudo /usr/sbin/service milter-manager start</pre>

/usr/local/bin/milter-test-server is usuful to confirmmilter-manager was ran:

<pre>% sudo -u mailnull milter-test-server -s unix:/var/run/milter-manager/milter-manager.sock</pre>

Here is a sample success output:

<pre>status: pass
elapsed-time: 0.128 seconds</pre>

If milter-manager fails to run, the following message willbe shown:

<pre>Failed to connect to unix:/var/run/milter-manager/milter-manager.sock: No such file or directory</pre>

In this case, we can use log to solve theproblem. milter-manager is verbosily if --verbose option isspecified. milter-manager outputs logs to standard output ifmilter-manager isn't daemon process.

We add the following to /etc/rc.conf to output verbose logto standard output:

<pre>miltermanager_debug="YES"</pre>

milter-manager should be started:

<pre>% sudo /usr/sbin/service milter-manager restart</pre>

Some logs are output if there is a problem. Runningmilter-manager can be exitted by Ctrl+c.

milter_manager_debug configuration in /etc/rc.conf should becommented out after the problem is solved to runmilter-manager as daemon process. And milter-manager shouldrestarted.

### Configure Postfix

We add 'postfix' user to 'mail' group:

<pre>% sudo /usr/sbin/pw groupmod mail -m postfix</pre>

We start milter's configuration.

We add the following milter configuration to/usr/local/etc/postfix/main.cf:

<pre>milter_protocol = 6
milter_default_action = accept
milter_mail_macros = {auth_author} {auth_type} {auth_authen}</pre>

Here are descriptions of the configuration.

<dl>
<dt>milter_protocol = 6</dt>
<dd>Postfix uses milter protocol version 6.</dd>
<dt>milter_default_action = accept</dt>
<dd>Postfix accepts a mail if Postfix can't connect tomilter. It's useful configuration for not stopping mailserver function if milter has some problems. But itcauses some problems that spam mails and virus mails maybe delivered until milter is recovered.

If you can recover milter, 'tempfail' will be betterchoice rather than 'accept'. Default is 'tempfail'.</dd>
<dt>milter_mail_macros = {auth_author} {auth_type} {auth_authen}</dt>
<dd>Postfix passes SMTP Auth related infomation tomilter. Some milters like milter-greylist use it.</dd></dl>

We need to register milter-manager to Postfix. It'simportant that spamass-milter, clamav-milter,milter-greylist aren't needed to be registered because theyare used via milter-manager.

We add the following to /usr/local/etc/postfix/main.cf:

<pre>smtpd_milters = unix:/var/run/milter-manager/milter-manager.sock</pre>

Postfix should reload its configuration:

<pre>% sudo /usr/sbin/service postfix reload</pre>

Postfix's milter configuration is completed.

milter-manager logs to syslog. If milter-manager works well,some logs can be showen in /var/log/maillog. We need to senta test mail for confirming.

## Conclusion

There are many configurations to work milter and Postfixtogether. They can be reduced by introducing milter-manager.

Without milter-manager, we need to specify sockets ofspamass-milter, clamav-milter and milter-greylist tosmtpd_milters. With milter-manager, we doesn't need tospecify sockets of them, just specify a coket ofmilter-manager. They are detected automatically. We doesn'tneed to take care some small mistakes like typo.

milter-manager also supports xxx_enabled="NO" configurationused in /etc/rc.conf. If we disable a milter, we use thefollowing steps:

<pre>% sudo /usr/sbin/service XXX stop
% sudo vim /etc/rc.conf # XXX_enabled="YES" => XXX_enabled="NO"</pre>

We need to reload milter-manager after we disable a milter.

<pre>% sudo /usr/sbin/service milter-manager reload</pre>

milter-manager detects a milter is disabled and doesn't useit. We doesn't need to change Postfix's main.cf.

We can reduce maintainance cost by introducingmilter-manager if we use some milters on FreeBSD.

milter manager also provides tools to helpoperation. Installing them is optional but we can reduceoperation cost too. If we also install them, we will go toInstall to FreeBSD(optional).


