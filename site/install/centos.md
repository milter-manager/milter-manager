---
title: Install to CentOS 8
---

# Install to CentOS 8 --- How to install milter manager to CentOS 8

## About this document

This document describes how to install milter manager to CentOS 8.See Install for general installinformation.

In this document, CentOS 8.3 is used. Sudo is used to run a commandwith root privilege. If you don't use sudo, use su instead.

## Install packages

Postfix is used as MTA because it's installed by default.

Spamass-milter, clamav-milter and milter-greylist are used asmilters. Milter packages registered in EPEL are used.

Register EPEL like the following:

<pre>% sudo dnf install -y epel-release</pre>

Now, you install milters:

<pre>% sudo dnf install -y spamass-milter-postfix clamav-scanner-systemd clamav-update clamav-milter clamav-milter-systemd milter-greylist</pre>

And you install RRDtool for generating graphs:

<pre>% sudo dnf install -y rrdtool</pre>

## Build and Install

milter manager can be installed by dnf.

Register milter manager yum repository like the following:

<pre>% curl -s https://packagecloud.io/install/repositories/milter-manager/repos/script.rpm.sh | sudo bash</pre>

See also: https://packagecloud.io/milter-manager/repos/install

Enable <code>ruby:3.0</code> module:

<pre>% sudo dnf module -y enable ruby:3.0</pre>

Now, you install milter manager:

<pre>% sudo dnf install -y milter-manager</pre>

## Configuration

Here is a basic configuration policy.

milter-greylist should be applied only if[S25R](http://gabacho.reto.jp/en/anti-spam/)condition is matched to reduce needless delivery delay.But the configuration is automatically done bymilter manager. You need to do nothing for it.

It's difficult that milter manager runs on SELinux. Disable SELinuxpolicy module for Postfix and Milter.

<pre>% sudo semodule -d postfix
% sudo semodule -d milter</pre>

### Configure spamass-milter

At first, you configure spamd.

spamd adds "[SPAM]" to spam mail's subject by default. Ifyou don't like the behavior, edit/etc/mail/spamassassin/local.cf.

Before:

<pre>rewrite_header Subject [SPAM]</pre>

After:

<pre># rewrite_header Subject [SPAM]</pre>

Add the following configuration to/etc/mail/spamassassin/local.cf. This configuration is for addingheaders only if spam detected.

<pre>remove_header ham Status
remove_header ham Level</pre>

Start spamd on startup:

<pre>% sudo systemctl enable spamassassin</pre>

Start spamd:

<pre>% sudo systemctl start spamassassin</pre>

Here are spamass-milter's configuration items:

* Disable needless body change feature.
* Reject if score is larger than or equal to 15.

Change /etc/sysconfig/spamass-milter:

Before:

<pre>#EXTRA_FLAGS="-m -r 15"</pre>

After:

<pre>EXTRA_FLAGS="-m -r 15"</pre>

Start spamass-milter on startup:

<pre>% sudo systemctl enable spamass-milter</pre>

Start spamass-milter:

<pre>% sudo systemctl start spamass-milter</pre>

### Configure clamav-milter

Update ClamAV virus database and start clamd.

Edit /etc/freshclam.conf like the following. It comments out"Example", changes "NotifyClamd" value and uncomments otheritems.

Before:

<pre>Example
#LogFacility LOG_MAIL
#NotifyClamd /path/to/clamd.conf</pre>

After:

<pre>#Example
LogFacility LOG_MAIL
NotifyClamd /etc/clamd.d/scan.conf</pre>

Run freshclam by hand at the first time:

<pre>% sudo freshclam</pre>

Configure clamd.

Edit /etc/clamd.d/scan.conf like the following. It comments out"Example" and uncomments other items:

Before:

<pre>Example
#LogFacility LOG_MAIL
#LocalSocket /run/clamd.scan/clamd.sock</pre>

After:

<pre>#Example
LogFacility LOG_MAIL
LocalSocket /run/clamd.scan/clamd.sock</pre>

Start clamd on startup:

<pre>% sudo systemctl enable clamd@scan</pre>

Start clamd:

<pre>% sudo systemctl start clamd@scan</pre>

Configure clamav-milter.

Edit /etc/mail/clamav-milter.conf like the following. It comments out"Example", change "ClamdSocket" value and uncomments other items:

Before:

<pre>Example
#MilterSocket /run/clamav-milter/clamav-milter.socket
#MilterSocketMode 660
#ClamdSocket tcp:scanner.mydomain:7357
#LogFacility LOG_MAIL</pre>

After:

<pre>#Example
MilterSocket /run/clamav-milter/clamav-milter.socket
MilterSocketMode 660
ClamdSocket unix:/run/clamd.scan/clamd.sock
LogFacility LOG_MAIL</pre>

Add "clamilt" user to "clamscan" group to access clamd's socket:

<pre>% sudo usermod -G clamscan -a clamilt</pre>

Start clamav-milter on startup:

<pre>% sudo systemctl enable clamav-milter</pre>

Start clamav-milter:

<pre>% sudo systemctl start clamav-milter</pre>

### Configure milter-greylist

Change /etc/mail/greylist.conf for the followingconfigurations:

* use the leading 24bits for IP address match to avoid    Greylist adverse effect for sender uses some MTA case.
* decrease retransmit check time to 10 minutes from 30    minutes (default value) to avoid Greylist adverse effect.
* increase auto whitelist period to a week from 1 day    (default value) to avoid Greylist adverse effect.
* don't use Greylist when trusted domain passes SPF.    (Trusted domains are configured in milter manager)
* use Greylist by default.

<pre># note
The configuration relaxes Greylist check to avoid Greylist
adverse effect. It increases received spam mails but you
should give priority to avoid false positive rather than
false negative. You should not consider that you blocks all
spam mails by Greylist. You can blocks spam mails that
isn't blocked by Greylist by other anti-spam technique
such as SpamAssassin. milter manager helps constructing
mail system that combines some anti-spam techniques.</pre>

Before:

<pre>socket "/run/milter-greylist/milter-greylist.sock"
# ...
racl whitelist default</pre>

After:

<pre>socket "/run/milter-greylist/milter-greylist.sock" 660
# ...
subnetmatch /24
greylist 10m
autowhite 1w
sm_macro "trusted_domain" "{trusted_domain}" "yes"
racl whitelist sm_macro "trusted_domain" spf pass
racl greylist sm_macro "trusted_domain" not spf pass
racl greylist default</pre>

Start milter-greylist on startup:

<pre>% sudo systemctl enable milter-greylist</pre>

Start milter-greylist:

<pre>% sudo systemctl start milter-greylist</pre>

### Configure milter manager

Add "milter-manager" user to "clamilt" group to accessclamav-milter's socket:

<pre>% sudo usermod -G clamilt -a milter-manager</pre>

Add "milter-manager" user to "mail" group and "grmilter" group toaccess milter-greylist's socket:

<pre>% sudo usermod -G mail -a milter-manager
% sudo usermod -G grmilter -a milter-manager</pre>

Add "milter-manager" user to "postfix"" group to accessspamass-milter's socket:

<pre>% sudo usermod -G postfix -a milter-manager</pre>

milter manager detects milters that installed in system.You can confirm spamass-milter, clamav-milter andmilter-greylist are detected:

<pre>% sudo /usr/sbin/milter-manager -u milter-manager -g milter-manager --show-config</pre>

The following output shows milters are detected:

<pre>...
define_milter("milter-greylist") do |milter|
  milter.connection_spec = "unix:/run/milter-greylist/milter-greylist.sock"
  ...
  milter.enabled = true
  ...
end
...
define_milter("clamav-milter") do |milter|
  milter.connection_spec = "unix:/var/run/clamav-milter/clamav-milter.socket"
  ...
  milter.enabled = true
  ...
end
...
define_milter("spamass-milter") do |milter|
  milter.connection_spec = "unix:/run/spamass-milter/postfix/sock"
  ...
  milter.enabled = true
  ...
end
...</pre>

You should confirm that milter's name, socket path and "enabled =true". If the values are unexpected, you need to change/etc/milter-manager/milter-manager.local.conf. SeeConfiguration for details ofmilter-manager.local.conf.

But if we can, we want to use milter manager without editingmiter-manager.local.conf. If you report your environment to the miltermanager project, the milter manager project may improve detect method.

milter manager's configuration is finished.

Start to milter manager on startup:

<pre>% sudo systemctl enable milter-manager</pre>

Start to milter manager:

<pre>% sudo systemctl start milter-manager</pre>

milter-test-server is usuful to confirm milter manager wasran:

<pre>% sudo -u milter-manager milter-test-server -s unix:/var/run/milter-manager/milter-manager.sock</pre>

Here is a sample success output:

<pre>status: accept
elapsed-time: 0.128 seconds</pre>

If milter manager fails to run, the following message willbe shown:

<pre>Failed to connect to unix:/var/run/milter-manager/milter-manager.sock</pre>

In this case, you can use log to solve theproblem. milter manager is verbosily if --verbose option isspecified. milter manager outputs logs to standard output ifmilter manager isn't daemon process.

You can add the following configuration to/etc/sysconfig/milter-manager to output verbose log tostandard output:

<pre>OPTION_ARGS="--verbose --no-daemon"</pre>

Restart milter manager:

<pre>% sudo systemctl restart milter-manager</pre>

Some logs are output if there is a problem. Runningmilter manager can be exitted by Ctrl+c.

OPTION_ARGS configuration in /etc/sysconfig/milter-managershould be commented out after the problem is solved to runmilter manager as daemon process. And you should restartmilter manager.

### Configure Postfix

Enables Postfix:

<pre>% sudo systemctl enable postfix
% sudo systemctl start postfix</pre>

Configure Postfix for milters. Append following lines to/etc/postfix/main.cf:

<pre>milter_protocol = 6
milter_default_action = accept</pre>

For details for each lines.

<dl>
<dt>milter_protocol = 6</dt>
<dd>Use milter protocol version 6.</dd>
<dt>milter_default_action = accept</dt>
<dd>MTA receives mails when MTA cannot access milter. Although thereare problems between MTA and milter, MTA can deliver mails to clients.But until you recover milter, perhaps MTA receives spam mails andvirus mails.

If you can recover the system quickly, you can specify 'tempfail'instead of 'accept'. Default value is 'tempfail'.</dd>
<dt>milter_mail_macros = {auth_author} {auth_type} {auth_authen}</dt>
<dd>MTA gives information related SMTP Auth to milters. milter-greylistetc. uses it.</dd></dl>

Register milter manager to Postfix.  It's important thatspamass-milter, clamav-milter and milter-greylist aren't needed to beregistered because they are used via milter manager.

Append following lines to /etc/postfix/main.cf:

<pre>smtpd_milters = unix:/var/run/milter-manager/milter-manager.sock</pre>

Reload Postfix's configuration.

<pre>% sudo systemctl reload postfix</pre>

milter manager logs to syslog. If milter manager works well,some logs can be shown in /var/log/maillog. You need to senda test mail for confirming.

## Conclusion

There are many configurations to work milter and Postfixtogether. They can be reduced by introducing milter manager.

Without milter manager, you need to specify sockets ofspamass-milter, clamav-milter and milter-greylist to/etc/postfix/main.cf. With milter manager, you don't need tospecify sockets of them, just specify a socket ofmilter manager. They are detected automatically. You don'tneed to take care some small mistakes like typo.

milter manager also detects which '/sbin/chkconfig -add' isdone or not. If you disable a milter, you use the followingsteps:

<pre>% sudo systemctl stop milter-greylist
% sudo systemctl disable milter-greylist</pre>

You need to reload milter manager after you disable a milter.

<pre>% sudo systemctl reload milter-manager</pre>

milter manager detects a milter is disabled and doesn't useit. You don't need to change /etc/postfix/main.cf.

You can reduce maintainance cost by introducingmilter manager if you use some milters on CentOS.

milter manager also provides tools to helpoperation. Installing them is optional but you can reduceoperation cost too. If you also install them, you will go toInstall to CentOS(optional).


