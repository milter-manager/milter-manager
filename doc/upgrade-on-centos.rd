# -*- rd -*-

= Upgrade on CentOS --- How to upgrade milter manager on CentOS

== About this document

This document describes how to upgrade milter manager on
CentOS. See ((<Install to CentOS|install-to-centos.rd>)) for
newly install information.

== Upgrade

If we are using 1.8.1 or earlier, we need to install
milter-manager-release at first. If we are using 1.8.2 or
later, we doesn't need to this operation.

We need to disable GPG key check because the new repository
uses a new GPG key and a new GPG key is included in the
package.

  % sudo yum install --nogpgcheck -y milter-manager-release

We just upgrade milter manager package.
  % sudo yum update -y milter-manager

== Conclusion

milter manager can be upgraded easily. It means that milter
manager is a low maintenance cost software.

If we want to use improvements in a new version, please
upgrade your milter manager.

If we've also installed optional packages, we will go to
((<Upgrade on CentOS
(optional)|upgrade-options-on-centos.rd>)).
