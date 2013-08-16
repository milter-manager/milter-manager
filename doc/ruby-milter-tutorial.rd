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

== Summary

Milter written in Ruby is followings:

  require 'milter/client'

  class Session < Milter::ClientSession
    def initialize(context)
      super(context)
      # Initialize
    end

    def connect(host, address)
      # ...
    end

    # Other callback definitions
  end

  command_line = Milter::Client::CommandLine.new
  command_line.run do |client, _options|
    client.register(Session)
  end

Let's write the milter that can reject a mail includes specified
regular expression.

== Callbacks

Milter callback methods are called for each event. Almost events have
additional information. You can pass additional information via
callback parameters or macro. This document describes callback
parameters.

This is the list of callback methods and parameters.

: connect(host, address)

   This method is called when SMTP client connects to SMTP server.

   ((|host|)) is hostname of SMTP client.
   ((|address|)) is IP Address of SMTP client.

   For example, connetct from localhost.

   : host
       "localhost"
   : address
       This is a (({Milter::SocketAddress::IPv4})) object represents
       (({inet:45875@[127.0.0.1]})).

: helo(fqdn)

   This method is called when SMTP client sends HELO or EHLO command.

   ((|fqdn|)) is FQDN reported via HELO/EHLO.

   For example, SMTP client sends "EHLO mail.example.com".

   : fqdn
      "mail.example.com"

: envelope_from(from)

   This method is called when SMTP client sends MAIL command.

   ((|from|)) is sender mail address reported via MAIL command.

   For exapme, SMTP client sends "MAIL FROM: <user@example.com>"

   : from
      "<user@example.com>"

: envelope_recipient(to)

   This method is called when SMTP client send RCPT command.
   This method is called twice if SMTP client send RCPT command twice.

   ((|to|)) is recipient mail address reported via RCPT command.

   For example, SMTP client sends "RCPT TO: <user@example.com>"

   : to
      "<user@example.com>"

: data

   This method is called when SMTP client sends DATA command.

: header(name, value)

   This method is called N times. N is the number of headers included
   in the mail.

   ((|name|)) is header name.
   ((|value|)) is header value.

   For example, there is a header which is "Subject: Hello!"

   : name
      "Subject"

   : value
      "Hello!"

: end_of_header

   This method is called when milter has finished processing header of
   the mail.

: body(chunk)

   This method is called when milter has received mail body.
   This method is called only once if mail body is small enough.
   This methos is called multiple times if mail body is large.

   ((|chunk|)) is splitted body.

   For examle, if the mail body includes "Hi!", this method is called only once.

   : chunk
      "Hi!"

: end_of_message

   This method is called when SMTP client sends "<CR><LF>.<CR><LF>"
   that represents end of data.

: abort(state)

   This method is called when SMTP transaction is resetted.
   In particular, after end_of_message and SMTP client sends RSET.

   ((|state|)) is a object represents timing of calling abort.

: unknown(command)

   This method is called when SMTP client sends unknown command in
   milter protocol.

   ((|command|)) is command name.

: reset

   This method is called when initialize and finish mail transaction.

   ((mail transaction|URL:http://tools.ietf.org/html/rfc5321#section-3.3>))
   has finished at:

     * Called (({abort})) callback.
     * Call (({reject})) in milter.
     * Call (({temporary_failure})) in milter.
     * Call (({discard})) in milter.
     * Call (({accept})) in milter.

: finished

   This method is called when completed milter protocol.
   TODO: write about timing

== Using callbacks

We want to write the milter which will reject mails match against
specified regular expression. The regular expression matches against
subject and message body. It is necessary for us to select header
callback and body callback. Template is as following.

  require 'milter/client'

  class MilterRegexp < Milter::ClientSession
    def initialize(context, regexp)
      super(context)
      @regexp = regexp
    end

    def header(name, value)
      # ... Check subject header
    end

    def body(chunk)
      # check chunk
    end
  end

  command_line = Milter::Client::CommandLine.new
  command_line.run do |client, _options|
    # We want to reject mails include "viagra"
    client.register(MilterRegexp, /viagra/i)
  end

== Check subject header

First, let's check subject header.

  class MilterRegexp < Milter::ClientSession
    # ...
    def header(name, value)
      case name
      when /\ASubject\z/i
        if @regexp =~ value
          reject
        end
      end
    end
    # ...
  end

Reject mails if header name is matched "subject" and its value matches
against specified regular expression.


