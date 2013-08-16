# -*- rd -*-

= milter development with Ruby --- A tutorial for Ruby bindings

== About this document

This document describes how to develop a milter with Ruby
bindings for the milter library provided by milter manager.

See ((<Developer center|URL:https://www.milter.org/developers>))
abount milter protocol.

== Install

You can specify --enable-ruby-milter option to configure script if you
want to develop milter writing Ruby. You can install packages on
Debian GNU/Linux, Ubuntu and CentOS because there are deb/rpm packages
for those platforms.

Debian GNU/Linux or Ubuntu:

  % sudo aptitude -V -D -y install ruby-milter-core ruby-milter-client ruby-milter-server

CentOS:

  % sudo yum install -y ruby-milter-core ruby-milter-client ruby-milter-server

You can specify --enable-ruby-milter option to configure script if
there are no package in your environment.

  % ./configure --enable-ruby-milter

You can confirm installed library version.

  % ruby -r milter -e 'p Milter::VERSION'
  [1, 8, 0]

You have succeeded to install ruby-milter if you can see version
information.

