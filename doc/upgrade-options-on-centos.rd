# -*- rd -*-

= Upgrade on CentOS (optional) --- How to upgrade milter manager related softwares on CentOS

== About this document

This document describes how to upgrade milter manager on
CentOS. See ((<Install to CentOS
(optional)|install-to-CentOS.rd>)) for newly install
information.

You need to upgrade milter manager before you upgrade
optional packages.  See ((<Upgrade on
CentOS|upgrade-on-CentOS.rd>)) if you don't upgrade milter
manager yet.

== Upgrade milter-manager-log-analyzer

You already upgrade milter-manager-log-analyzer when you
upgrade milter-manager package.

== Upgrade milter manager admin

=== Upgrade gems

  % sudo gem install rails -v '2.3.4'
  % sudo gem install passenger -v '2.2.5'

=== Upgrade Passenger

To build Passenger we run the following command:

  % (echo 1; echo) | sudo passenger-install-apache2-module

We upgrade Passenger version in
/etc/httpd/conf.d/milter-manager.conf to 2.2.5:

/etc/httpd/conf.d/milter-manager.conf:
  LoadModule passenger_module /usr/lib/ruby/gems/1.8/gems/passenger-2.2.5/ext/apache2/mod_passenger.so
  PassengerRoot /usr/lib/ruby/gems/1.8/gems/passenger-2.2.5
  PassengerRuby /usr/bin/ruby

  RailsBaseURI /milter-manager

We restart Apache:

  % sudo /sbin/service httpd reload

=== Upgrade milter manager admin

We upgrade milter manager admin and its database schema:

  % tar cf - -C /usr/share/milter-manager admin | sudo -u milter-manager -H tar xf - -C ~milter-manager
  % cd ~milter-manager/admin
  % sudo -u milter-manager -H rake gems:install
  % sudo -u milter-manager -H rake RAILS_ENV=production db:migrate

== Conclusion

We can upgrade milter manager related softwares as easily as
milter manager. We can upgrade if we want improvements in
newer version.
