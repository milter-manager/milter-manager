# -*- rd -*-

= Upgrade on CentOS --- How to upgrade milter manager on CentOS

== About this document

This document describes how to upgrade milter manager on
CentOS. See ((<Install to CentOS|install-to-centos.rd>)) for
newly install information.

== Upgrade

We just upgrade milter manager package.

On 32bit environment:

  % wget http://downloads.sourceforge.net/milter-manager/milter-manager-1.3.1-0.i386.rpm
  % sudo rpm -Uvh milter-manager-1.3.1-0.i386.rpm

On 64bit environment:

  % wget http://downloads.sourceforge.net/milter-manager/milter-manager-1.3.1-0.x86_64.rpm
  % sudo rpm -Uvh milter-manager-1.3.1-0.x86_64.rpm

== Conclusion

milter manager can be upgraded easily. It means that milter
manager is a low maintenance cost software.

If you want to use improvements in a new version, please
upgrade your milter manager.
