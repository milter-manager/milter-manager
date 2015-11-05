# -*- rd -*-

= Upgrade on Ubuntu --- How to upgrade milter manager on Ubuntu Linux

== About this document

This document describes how to upgrade milter manager on
Ubuntu Linux. See ((<Install to
Ubuntu|install-to-ubuntu.rd>)) for newly install
information.

== Upgrade

We just upgrade milter manager package.

  % sudo apt-get -V -y upgrade

=== Upgrade from before 1.8.0

We have changed the GPG key to sign packages on 2011-11-15.
Please import new GPG key.

  % sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 1BD22CD1

We have changed sources.list URI. We are going to delete old URI
support, when we release milter manager 1.9.0.

Please update your source.list as soon as possible.

=== For Precise Pangolin

/etc/apt/sources.list.d/milter-manager.list:
  deb http://sourceforge.net/projects/milter-manager/files/ubuntu/stable precise universe
  deb-src http://sourceforge.net/projects/milter-manager/files/ubuntu/stable precise universe

=== For Saucy Salamander

/etc/apt/sources.list.d/milter-manager.list:
  deb http://sourceforge.net/projects/milter-manager/files/ubuntu/stable saucy universe
  deb-src http://sourceforge.net/projects/milter-manager/files/ubuntu/stable saucy universe

=== For Trusty Tahr

/etc/apt/sources.list.d/milter-manager.list:
  deb http://sourceforge.net/projects/milter-manager/files/ubuntu/stable trusty universe
  deb-src http://sourceforge.net/projects/milter-manager/files/ubuntu/stable trusty universe

=== For Utopic Unicorn

/etc/apt/sources.list.d/milter-manager.list:
  deb http://sourceforge.net/projects/milter-manager/files/ubuntu/stable utopic universe
  deb-src http://sourceforge.net/projects/milter-manager/files/ubuntu/stable utopic universe

=== For Vivid Vervet

/etc/apt/sources.list.d/milter-manager.list:
  deb http://sourceforge.net/projects/milter-manager/files/ubuntu/stable vivid universe
  deb-src http://sourceforge.net/projects/milter-manager/files/ubuntu/stable vivid universe

=== For Wily Werewolf

/etc/apt/sources.list.d/milter-manager.list:
  deb http://sourceforge.net/projects/milter-manager/files/ubuntu/stable wily universe
  deb-src http://sourceforge.net/projects/milter-manager/files/ubuntu/stable wily universe

== Conclusion

milter manager can be upgraded easily. It means that milter
manager is a low maintenance cost software.

If we want to use improvements in a new version, please
upgrade your milter manager.

If we've also installed optional packages, we will go to
((<Upgrade on Ubuntu
(optional)|upgrade-options-on-ubuntu.rd>)).
