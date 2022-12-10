---
title: Install to CentOS 6
---

# Install to CentOS 6 --- How to install milter manager to CentOS 6

## About this document

This document describes how to install milter manager to CentOS 6. SeeInstall to CentOS 7 for CentOS 7 specificinstall information. SeeInstall to CentOS 5 for CentOS 5 specificinstall information. See Install for general installinformation.

In this document, CentOS 6.4 is used. Sudo is used to run a commandwith root privilege. If you don't use sudo, use su instead.

## Install packages

Postfix is used as MTA because it's installed by default.

Spamass-milter, clamav-milter and milter-greylist are used asmilters. Milter packages registered in EPEL are used.

Register EPEL like the following.

On 32bit environment:

<pre>% sudo yum install -y http://ftp.jaist.ac.jp/pub/Linux/Fedora/epel/6/i386/epel-release-6-8.noarch.rpm</pre>

On 64bit environment:

<pre>% sudo yum install -y http://ftp.jaist.ac.jp/pub/Linux/Fedora/epel/6/x86_64/epel-release-6-8.noarch.rpm</pre>

Now, you can install milters:

<pre>% sudo yum install -y spamass-milter clamav-milter milter-greylist</pre>

And you can install RRDtool for generating graphs:

<pre>% sudo yum install -y rrdtool</pre>

## Build and Install

milter manager can be installed by yum.

Register milter manager yum repository like the following:

<pre>% curl -s https://packagecloud.io/install/repositories/milter-manager/repos/script.rpm.sh | sudo bash</pre>

See also: https://packagecloud.io/milter-manager/repos/install

Now, you can install milter manager:

<pre>% sudo yum install -y milter-manager</pre>

## Configuration

Here is a basic configuration policy.

IPv4 socket is used and connections only from localhost are accepted.

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

Add the following configuration to/etc/spamassassin/local.cf. This configuration is for addingheaders only if spam detected.

<pre>remove_header ham Status
remove_header ham Level</pre>

Start spamd on startup:

<pre>% sudo /sbin/chkconfig spamassassin on</pre>

Start spamd:

<pre>% sudo run_init /sbin/service spamassassin start</pre>

Here are spamass-milter's configuration items:

* Change socket address.
* Disable needless body change feature.
* Reject if score is larger than or equal to 15.

Change /etc/sysconfig/spamass-milter:

Before:

<pre>#SOCKET=/var/run/spamass.sock
#EXTRA_FLAGS="-m -r 15"</pre>

After:

<pre>SOCKET="inet:11120@[127.0.0.1]"
EXTRA_FLAGS="-m -r 15"</pre>

Start spamass-milter on startup:

<pre>% sudo /sbin/chkconfig spamass-milter on</pre>

Start spamass-milter:

<pre>% sudo run_init /sbin/service spamass-milter start</pre>

### Configure clamav-milter

Update ClamAV virus database and start clamd.

<pre>% sudo freshclam
% sudo run_init /sbin/service clamd start</pre>

Edit /etc/clamav-milter.conf to change clamav-milter'ssocket address.

Before:

<pre>#MilterSocketMode 0660</pre>

After:

<pre>MilterSocketMode 0660</pre>

Start clamav-milter:

<pre>% sudo run_init /sbin/service clamav-milter start</pre>

### Configure milter-greylist

Change /etc/mail/greylist.conf for the followingconfigurations:

* use the leading 24bits for IP address match to avoid    Greylist adverse effect for sender uses some MTA case.
* decrease retransmit check time to 10 minutes from 30    minutes (default value) to avoid Greylist adverse effect.
* increase auto whitelist period to a week from 1 day    (default value) to avoid Greylist adverse effect.
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

<pre>socket "/var/run/milter-greylist/milter-greylist.sock"
# ...
racl whitelist default</pre>

After:

<pre>socket "/var/run/milter-greylist/milter-greylist.sock" 660
# ...
subnetmatch /24
greylist 10m
autowhite 1w
racl greylist default</pre>

"grmilter", the effective user of milter-grylist, only can access/var/run/milter-greylist/ directory. To access the socket file in/var/run/milter-greylist/ directory by milter-manager, add permissionto the directory:

<pre>% sudo chmod +rx /var/run/milter-greylist/</pre>

Start milter-greylist on startup:

<pre>% sudo /sbin/chkconfig milter-greylist on</pre>

Start milter-greylist:

<pre>% sudo run_init /sbin/service milter-greylist start</pre>

### Configure milter manager

Add 'milter-manager' user to 'clam' group to accessclamav-milter's socket:

<pre>% sudo usermod -G clam -a milter-manager</pre>

Add 'milter-manager' user to 'grmilter' group to accessmilter-greylist's socket:

<pre>% sudo usermod -G grmilter -a milter-manager</pre>

milter manager detects milters that installed in system.You can confirm spamass-milter, clamav-milter andmilter-greylist are detected:

<pre>% sudo /usr/sbin/milter-manager -u milter-manager --show-config</pre>

The following output shows milters are detected:

<pre>...
define_milter("milter-greylist") do |milter|
  milter.connection_spec = "inet:11122@[127.0.0.1]"
  ...
  milter.enabled = true
  ...
end
...
define_milter("clamav-milter") do |milter|
  milter.connection_spec = "unix:/var/clamav/clmilter.socket"
  ...
  milter.enabled = true
  ...
end
...
define_milter("spamass-milter") do |milter|
  milter.connection_spec = "inet:11120@[127.0.0.1]"
  ...
  milter.enabled = true
  ...
end
...</pre>

You should confirm that milter's name, socket path and'enabled = true'. If the values are unexpected,you need to change/etc/milter-manager/milter-manager.conf.See Configuration for details ofmilter-manager.conf.

But if we can, we want to use milter manager without editingmiter-manager.conf. If you report your environment to themilter manager project, the milter manager project mayimprove detect method.

milter manager's configuration is finished. Start tomilter manager:

<pre>% sudo /sbin/service milter-manager restart</pre>

milter-test-server is usuful to confirm milter manager wasran:

<pre>% sudo -u milter-manager milter-test-server -s unix:/var/run/milter-manager/milter-manager.sock</pre>

Here is a sample success output:

<pre>status: pass
elapsed-time: 0.128 seconds</pre>

If milter manager fails to run, the following message willbe shown:

<pre>Failed to connect to unix:/var/run/milter-manager/milter-manager.sock</pre>

In this case, you can use log to solve theproblem. milter manager is verbosily if --verbose option isspecified. milter manager outputs logs to standard output ifmilter manager isn't daemon process.

You can add the following configuration to/etc/sysconfig/milter-manager to output verbose log tostandard output:

<pre>OPTION_ARGS="--verbose --no-daemon"</pre>

Restart milter manager:

<pre>% sudo run_init /sbin/service milter-manager restart</pre>

Some logs are output if there is a problem. Runningmilter manager can be exitted by Ctrl+c.

OPTION_ARGS configuration in /etc/sysconfig/milter-managershould be commented out after the problem is solved to runmilter manager as daemon process. And you should restartmilter manager.

### Configure Postfix

First, add 'postfix' user to 'milter-manager' group to accessmilter manager's socket:

<pre>% sudo usermod -G milter-manager -a postfix</pre>

Enables Postfix:

<pre>% sudo /sbin/chkconfig --add postfix
% sudo run_init /sbin/service postfix start</pre>

Configure Postfix for milters. Append following lines to/etc/postfix/main.cf:

<pre>milter_protocol = 6
milter_default_action = accept
milter_mail_macros = {auth_author} {auth_type} {auth_authen}</pre>

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

<pre>% sudo /etc/init.d/postfix reload</pre>

milter manager logs to syslog. If milter manager works well,some logs can be shown in /var/log/maillog. You need to senda test mail for confirming.

## Conclusion

There are many configurations to work milter and Postfixtogether. They can be reduced by introducing milter manager.

Without milter manager, you need to specify sockets ofspamass-milter, clamav-milter and milter-greylist to/etc/postfix/main.cf. With milter manager, you don't need tospecify sockets of them, just specify a socket ofmilter manager. They are detected automatically. You don'tneed to take care some small mistakes like typo.

milter manager also detects which '/sbin/chkconfig -add' isdone or not. If you disable a milter, you use the followingsteps:

<pre>% sudo /sbin/service milter-greylist stop
% sudo /sbin/chkconfig --del milter-greylist</pre>

You need to reload milter manager after you disable a milter.

<pre>% sudo /sbin/service milter-manager reload</pre>

milter manager detects a milter is disabled and doesn't useit. You don't need to change /etc/postfix/main.cf.

You can reduce maintainance cost by introducingmilter manager if you use some milters on CentOS.

milter manager also provides tools to helpoperation. Installing them is optional but you can reduceoperation cost too. If you also install them, you will go toInstall to CentOS(optional).


