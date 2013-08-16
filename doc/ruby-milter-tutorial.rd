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

== Operation check

Let's try to execute this milter.

Now, your milter is as following.

  require 'milter/client'

  class MilterRegexp < Milter::ClientSession
    def initialize(context, regexp)
      super(context)
      @regexp = regexp
    end

    def header(name, value)
      case name
      when /\ASubject\z/i
        if @regexp =~ value
          reject
        end
      end
    end

    def body(chunk)
      # Check cunk
    end
  end

  command_line = Milter::Client::CommandLine.new
  command_line.run do |client, _options|
    # We want to reject mails include "viagra"
    client.register(MilterRegexp, /viagra/i)
  end

You can execute this file as milter below command if you save this
file as "milter-regexp.rb". We add "-v" option because it is easy
to check operation.

  % ruby milter-regexp.rb -v

In this case (default), milter run in foreground. You can check
operation via other terminal.

((<milter-test-server>)) is very useful to test milter.
Milter which is written in Ruby is launched on "inet:20025@localhost".

  % milter-test-server -s inet:20025
  status: pass
  elapsed-time: 0.00254348 seconds

You can see "status: pass" in your terminal if you can connect properly.
Let's check another terminal.

  [2010-08-01T05:44:34.157419Z]: [client][accept] 10:inet:55651@127.0.0.1
  [2010-08-01T05:44:34.157748Z]: [1] [client][start]
  [2010-08-01T05:44:34.157812Z]: [1] [reader][watch] 4
  [2010-08-01T05:44:34.157839Z]: [1] [writer][watch] 5
  [2010-08-01T05:44:34.158050Z]: [1] [reader] reading from io channel...
  [2010-08-01T05:44:34.158140Z]: [1] [command-decoder][negotiate]
  [2010-08-01T05:44:34.158485Z]: [1] [client][reply][negotiate] #<MilterOption version=<6> action=<add-headers|change-body|add-envelope-recipient|delete-envelope-recipient|change-headers|quarantine|change-envelope-from|add-envelope-recipient-with-parameters|set-symbol-list> step=<no-connect|no-helo|no-envelope-from|no-envelope-recipient|no-end-of-header|no-unknown|no-data|skip|envelope-recipient-rejected>>
  [2010-08-01T05:44:34.158605Z]: [1] [client][reply][negotiate][continue]
  [2010-08-01T05:44:34.158895Z]: [1] [reader] reading from io channel...
  [2010-08-01T05:44:34.158970Z]: [1] [command-decoder][header] <From>=<<kou+send@example.com>>
  [2010-08-01T05:44:34.159092Z]: [1] [client][reply][header][continue]
  [2010-08-01T05:44:34.159207Z]: [1] [reader] reading from io channel...
  [2010-08-01T05:44:34.159269Z]: [1] [command-decoder][header] <To>=<<kou+receive@example.com>>
  [2010-08-01T05:44:34.159373Z]: [1] [client][reply][header][continue]
  [2010-08-01T05:44:34.159485Z]: [1] [reader] reading from io channel...
  [2010-08-01T05:44:34.159544Z]: [1] [command-decoder][body] <71>
  [2010-08-01T05:44:34.159656Z]: [1] [client][reply][body][continue]
  [2010-08-01T05:44:34.159774Z]: [1] [reader] reading from io channel...
  [2010-08-01T05:44:34.159842Z]: [1] [command-decoder][define-macro] <E>
  [2010-08-01T05:44:34.159882Z]: [1] [command-decoder][end-of-message] <0>
  [2010-08-01T05:44:34.159941Z]: [1] [client][reply][end-of-message][continue]
  [2010-08-01T05:44:34.160034Z]: [1] [command-decoder][quit]
  [2010-08-01T05:44:34.160081Z]: [1] [agent][shutdown]
  [2010-08-01T05:44:34.160118Z]: [1] [agent][shutdown][reader]
  [2010-08-01T05:44:34.160162Z]: [1] [reader][eof]
  [2010-08-01T05:44:34.160199Z]: [1] [reader] shutdown requested.
  [2010-08-01T05:44:34.160231Z]: [1] [reader] removing reader watcher.
  [2010-08-01T05:44:34.160299Z]: [1] [writer][shutdown]
  [2010-08-01T05:44:34.160393Z]: [0] [reader][dispose]
  [2010-08-01T05:44:34.160452Z]: [client][finisher][run]
  [2010-08-01T05:44:34.160492Z]: [1] [client][finish]
  [2010-08-01T05:44:34.160536Z]: [1] [client][rest] []
  [2010-08-01T05:44:34.160578Z]: [sessions][finished] 1(+1) 0

You cannot connect to milter if you can see nothing in this terminal.
Please check to launch milter or to specify correct address to
milter-test-server.

Let's check operation when milter process a mail included "viagra" in subject.
You can reproduce the mail as following command.

  % milter-test-server -s inet:20025 --header 'Subject:Buy viagra!!!'
  status: reject
  elapsed-time: 0.00144477 seconds

You can check expected result because you can see "status: reject" in
you terminal.

In another terminal, you can see log as followings.

  ...
  [2010-08-01T05:49:49.275257Z]: [2] [command-decoder][header] <Subject>=<Buy viagra!!!>
  [2010-08-01T05:49:49.275405Z]: [2] [client][reply][header][reject]
  ...

The mitler reject the mail when process subject header.

milter manager provides usuful tools and libraries.

== Check message body

Let's check message body.

  class MilterRegexp < Milter::ClientSession
    def body(chunk)
      if @regexp =~ chunk
        reject
      end
    end
  end

Reject mails if message body chunk matches against specified regular
expression.

Let's try. milter-test-server can specify message body via "--body"
option.

  % tool/milter-test-server -s inet:20025 --body 'Buy viagra!!!'
  status: reject
  elapsed-time: 0.00195496 seconds

It is expected result because you can see "status: reject" in your
terminal.

== Problems

There are some problems in this milter because this milter simplify
for this tutorial.


  (1) Include MIME encoded header.
      Decoded "=?ISO-2022-JP?B?GyRCJVAlJCUiJTAlaRsoQnZpYWdyYQ==?="
      includes "viagra", but original header value does not match
      against specified regular expression. And milter does not reject
      this mail
  (2) The word splitted by chunk in message body.
      For exapmle, Specified regular expression does not match if
      first chunk has "via" and second one has "gra". And milter does
      not reject this mail.

You can solve problems about header if you use NKF library as following.

  require 'nkf'

  class MilterRegexp < Milter::ClientSession
    # ...
    def header(name, value)
      case name
      when /\ASubject\z/i
        if @regexp =~ NKF.nkf("-w", value)
          reject
        end
      end
    end
    # ...
  end

You can solve problems about message body if milter check message body
when milter receives all chunks.

  class MilterRegexp < Milter::ClientSession
    ...
    def initialize(context, regexp)
      super(context)
      @regexp = regexp
      @body = ""
    end

    def body(chunk)
      if @regexp =~ chunk
        reject
      end
      @body << chunk
    end

    def end_of_mesasge
      if @regexp =~ @body
        reject
      end
    end
    ...
  end

You can test multiple chunks as following.

  % milter-test-server -s inet:20025 --body 'Buy via' --body 'gra!!!'
  status: reject
  elapsed-time: 0.00379063 seconds

Mails are rejected if it includes multiple chunks.

However, in this case, all messages place on memory. This is
performance problem. In addition, this milter does not work if message
body is BASE64 encoded.

((<Mail|URL:http://github.com/mikel/mail>)) is useful library that
handle mails.

== Conclusion

This document describes how to write milter in Ruby. It is easy to
write milter in Ruby. Let's try to write milter in Ruby!!
