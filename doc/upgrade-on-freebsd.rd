# -*- rd -*-

= Upgrade on FreeBSD --- How to upgrade milter manager on FreeBSD

== About this document

This document describes how to upgrade milter manager on
FreeBSD. See ((<Install to FreeBSD|install-to-freebsd.rd>))
for newly install information.

== Upgrade

We can upgrade milter manager by overriding existing
installation.

NOTE: /usr/local/etc/milter-manager/milter-manager.conf is
overridden. If you changed it, please backup it before
upgrading. If you are using milter-manager.local.conf not
milter-manager.conf, you don't need to backup
milter-manager.conf.

  % sudo pkg update
  % sudo pkg upgrade --yes milter-manager

== Conclusion

milter manager can be upgraded easily. It means that milter
manager is a low maintenance cost software.

If we want to use improvements in a new version, please
upgrade your milter manager.

If we've also installed optional packages, we will go to
((<Upgrade on FreeBSD
(optional)|upgrade-options-on-freebsd.rd>)).
