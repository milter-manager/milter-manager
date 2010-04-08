# -*- rd -*-

= Upgrade on FreeBSD (optional) --- How to upgrade milter manager related softwares on FreeBSD

== About this document

This document describes how to upgrade milter manager on
FreeBSD. See ((<Install to FreeBSD
(optional)|install-to-freebsd.rd>)) for newly install
information.

You need to upgrade milter manager before you upgrade
optional packages.  See ((<Upgrade on
FreeBSD|upgrade-on-freebsd.rd>)) if you don't upgrade milter
manager yet.

== Upgrade milter-manager-log-analyzer

You already upgrade milter-manager-log-analyzer when you
upgrade milter-manager package.

== Upgrade milter manager admin

=== Upgrade gems

  % sudo gem install rack -v '1.0.1'
  % sudo gem install rails -v '2.3.5'
  % sudo gem install passenger -v '2.2.11'

=== Upgrade Passenger

To build Passenger we run the following command:

  % (echo 1; echo) | sudo passenger-install-apache2-module

We upgrade Passenger version in
/usr/local/etc/apache22/Includes/passenger.conf to 2.2.11:

/usr/local/etc/apache22/Includes/passenger.conf:
  LoadModule passenger_module /usr/local/lib/ruby/gems/1.8/gems/passenger-2.2.11/ext/apache2/mod_passenger.so
  PassengerRoot /usr/local/lib/ruby/gems/1.8/gems/passenger-2.2.11
  PassengerRuby /usr/local/bin/ruby18

  RailsBaseURI /milter-manager

We restart Apache:

  % sudo /usr/local/etc/rc.d/apache22 restart

=== Upgrade milter manager admin

We upgrade milter manager admin and its database schema:

  % tar cf - -C /usr/share/milter-manager admin | sudo -u milter-manager -H tar xf - -C ~milter-manager
  % cd ~milter-manager/admin
  % sudo -u milter-manager -H rake gems:install
  % sudo -u milter-manager -H rake RAILS_ENV=production db:migrate
  % sudo -u milter-manager -H touch tmp/restart.txt

== Conclusion

We can upgrade milter manager related softwares as easily as
milter manager. We can upgrade if we want improvements in
newer version.
