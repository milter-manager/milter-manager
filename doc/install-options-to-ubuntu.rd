# -*- rd -*-

= Install to Ubuntu Linux (optional) --- How to install milter manager related softwares to Ubuntu Linux

== About this document

This document describes how to install milter manager
related softwares to Ubuntu Linux. See ((<Install to Ubuntu
Linux|install-to-ubuntu.rd>)) for milter manager install
information and ((<Install|install.rd>)) for general install
information.

== Install milter-manager-log-analyzer

=== Install packages

We use Apache as Web server.

  % sudo aptitude -V -D install apache2

We use RRDtool for generating graphs.

  % sudo aptitude -V -D install rrdtool

=== Configure milter-manager-log-analyzer

milter-manager-log-analyzer generates graphs to
milter-manager user's home directory. They are published at
at http://localhost/~milter-manager/log/.

  % sudo -u milter-manager mkdir -p ~milter-manager/public_html/log

Apache publishes users' files.

  % sudo /usr/sbin/a2enmod userdir
  % sudo /etc/init.d/apache2 force-reload

Next, we set cron for milter-manager user up. We create a
symbolic link under /etc/cron.d/.

  % sudo ln -s /usr/local/etc/milter-manager/cron.d/debian/milter-manager-log /etc/cron.d/

On Ubuntu Linux, milter-manager logs to
/var/log/mail.info. /var/log/mail.info can be read only by
administration users. In cron file, we read
/var/log/mail.info as root and it is passed to
milter-manager-log-analyzer by
pipe. milter-manager-log-analyzer is ran as milter-manager
user not root.

milter-manager-log-analyzer is invoked every 5 minutes. We
can confirm it is invoked at /var/log/cron.

== Install milter manager admin

=== Install packages

To install the following packages, related packages are also
installed:

  % sudo aptitude -V -D install rdoc libopenssl-ruby apache2-threaded-dev libsqlite3-ruby

=== Install RubyGems

  % cd ~/src/
  % wget http://rubyforge.org/frs/download.php/45905/rubygems-1.3.1.tgz
  % tar xvzf rubygems-1.3.1.tgz
  % cd rubygems-1.3.1
  % sudo ruby setup.rb --no-format-executable

=== Instal gems

  % sudo gem install rails -v '2.2.2'
  % sudo gem install passenger

=== Install Passenger

To build Passenger we run the following command:

  % (echo 1; echo) | sudo passenger-install-apache2-module

We create passenger.load and passenger.conf under
/etc/apache2/mods-available/.

/etc/apache2/mods-available/passenger.load:
  LoadModule passenger_module /usr/lib/ruby/gems/1.8/gems/passenger-2.0.6/ext/apache2/mod_passenger.so

/etc/apache2/mods-available/passenger.conf:
  PassengerRoot /usr/lib/ruby/gems/1.8/gems/passenger-2.0.6
  PassengerRuby /usr/bin/ruby1.8

  RailsBaseURI /milter-manager

We enables the configuration and reload it.

  % sudo /usr/sbin/a2enmod passenger
  % sudo /etc/init.d/apache2 force-reload

milter manager admin has password authentication but it's
better that milter manager admin accepts connections only
from trusted hosts. For example, here is an example
configuration that accepts connections only from
localhost. We can use the configuration by appending it to
/etc/apache2/mods-available/passenger.conf.

  <Location /milter-manager>
    Allow from 127.0.0.1
    Deny from ALL
  </Location>

If we append the configuration, we should not forget to
reload configuration:

  % sudo /etc/init.d/apache2 force-reload

=== Configure milter manager admin

milter manager admin is installed to
/usr/local/share/milter-manager/admin/. We run it as
milter-manager user authority, and access it at
http://localhost/milter-manager/.

  % tar cf - -C /usr/local/share/milter-manager admin | sudo -u milter-manager -H tar xf - -C ~milter-manager
  % sudo ln -s ~milter-manager/admin/public /usr/local/www/apache22/data/milter-manager
  % cd ~milter-manager/admin
  % sudo -u milter-manager -H rake gems:install
  % sudo -u milter-manager -H rake RAILS_ENV=production db:migrate

Then we create a file to
~milter-manager/admin/config/initializers/relative_url_root.rb
with the following content:

~milter-manager/admin/config/initializers/relative_url_root.rb
  ActionController::Base.relative_url_root = "/milter-manager"

Now, we can access to http://localhost/milter-manager/. The
first work is registering a user. We will move to
milter-manager connection configuration page after register
a user. We can confirm where milter-manager accepts control
connection:

  % sudo -u milter-manager -H /usr/local/sbin/milter-manager --show-config | grep controller.connection_spec
  controller.connection_spec = "unix:/var/run/milter-manager/milter-manager-controller.sock"

We register confirmed value by browser. In the above case,
we select "unix" from "Type" at first. "Path" will be
appeared. We specify
"/var/run/milter-manager/milter-manager-controller.sock" to "Path".

We can confirm registered child milters and their
configuration by browser.

== Conclusion

We can confirm milter's effect visually by
milter-manager-log-analyzer. If we use Postfix as MTA, we
can compare with
((<Mailgraph|URL:http://mailgraph.schweikert.ch/>))'s graphs
to confirm milter's effect. We can use graphs generated by
milter-manager-log-analyzer effectively when we are trying
out a milter.

We can reduce administration cost by using milter manager
admin. Because we can change configurations without editing
configuration file.

It's convenient that we can enable and/or disable milters by
browser when we try out milters. We can use graphs generated
by milter-manager-log-analyzer to find what is the best milter
combination for our mail system.
