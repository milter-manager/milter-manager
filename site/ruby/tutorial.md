---
title: milter development with Ruby
---

# milter development with Ruby --- A tutorial for Ruby bindings

## About this document

This document describes how to develop a milter with Rubybindings for the milter library provided by milter manager.

See [Developer center](https://www.milter.org/developers)abount milter protocol.

## Install

You can specify --enable-ruby-milter option to configure script if youwant to develop milter writing Ruby. You can install packages onDebian GNU/Linux, Ubuntu and CentOS because there are deb/rpm packagesfor those platforms.

Debian GNU/Linux or Ubuntu:

<pre>% sudo aptitude -V -D -y install ruby-milter-core ruby-milter-client ruby-milter-server</pre>

CentOS:

<pre>% sudo yum install -y ruby-milter-core ruby-milter-client ruby-milter-server</pre>

You can specify --enable-ruby-milter option to configure script ifthere are no package in your environment.

<pre>% ./configure --enable-ruby-milter</pre>

You can confirm installed library version.

<pre>% ruby -r milter -e 'p Milter::VERSION'
[1, 8, 0]</pre>

You have succeeded to install ruby-milter if you can see versioninformation.

## Summary

Milter written in Ruby is followings:

<pre>require 'milter/client'

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
end</pre>

Let's write the milter that can reject a mail includes specifiedregular expression.

## Callbacks

Milter callback methods are called for each event. Almost events haveadditional information. You can pass additional information viacallback parameters or macro. This document describes callbackparameters.

This is the list of callback methods and parameters.

<dl>
<dt>connect(host, address)</dt>
<dd>This method is called when SMTP client connects to SMTP server.

<var>host</var> is hostname of SMTP client.<var>address</var> is IP Address of SMTP client.

For example, connetct from localhost.

<dl>
<dt>host</dt>
<dd>"localhost"</dd>
<dt>address</dt>
<dd>This is a <code>Milter::SocketAddress::IPv4</code> object represents<code>inet:45875@[127.0.0.1]</code>.</dd></dl></dd>
<dt>helo(fqdn)</dt>
<dd>This method is called when SMTP client sends HELO or EHLO command.

<var>fqdn</var> is FQDN reported via HELO/EHLO.

For example, SMTP client sends "EHLO mail.example.com".

<dl>
<dt>fqdn</dt>
<dd>"mail.example.com"</dd></dl></dd>
<dt>envelope_from(from)</dt>
<dd>This method is called when SMTP client sends MAIL command.

<var>from</var> is sender mail address reported via MAIL command.

For exapme, SMTP client sends "MAIL FROM: &lt;user@example.com&gt;"

<dl>
<dt>from</dt>
<dd>"&lt;user@example.com&gt;"</dd></dl></dd>
<dt>envelope_recipient(to)</dt>
<dd>This method is called when SMTP client send RCPT command.This method is called twice if SMTP client send RCPT command twice.

<var>to</var> is recipient mail address reported via RCPT command.

For example, SMTP client sends "RCPT TO: &lt;user@example.com&gt;"

<dl>
<dt>to</dt>
<dd>"&lt;user@example.com&gt;"</dd></dl></dd>
<dt>data</dt>
<dd>This method is called when SMTP client sends DATA command.</dd>
<dt>header(name, value)</dt>
<dd>This method is called N times. N is the number of headers includedin the mail.

<var>name</var> is header name.<var>value</var> is header value.

For example, there is a header which is "Subject: Hello!"

<dl>
<dt>name</dt>
<dd>"Subject"</dd>
<dt>value</dt>
<dd>"Hello!"</dd></dl></dd>
<dt>end_of_header</dt>
<dd>This method is called when milter has finished processing header ofthe mail.</dd>
<dt>body(chunk)</dt>
<dd>This method is called when milter has received mail body.This method is called only once if mail body is small enough.This methos is called multiple times if mail body is large.

<var>chunk</var> is splitted body.

For examle, if the mail body includes "Hi!", this method is called only once.

<dl>
<dt>chunk</dt>
<dd>"Hi!"</dd></dl></dd>
<dt>end_of_message</dt>
<dd>This method is called when SMTP client sends "&lt;CR&gt;&lt;LF&gt;.&lt;CR&gt;&lt;LF&gt;"that represents end of data.</dd>
<dt>abort(state)</dt>
<dd>This method is called when SMTP transaction is resetted.In particular, after end_of_message and SMTP client sends RSET.

<var>state</var> is a object represents timing of calling abort.</dd>
<dt>unknown(command)</dt>
<dd>This method is called when SMTP client sends unknown command inmilter protocol.

<var>command</var> is command name.</dd>
<dt>reset</dt>
<dd>This method is called when initialize and finish mail transaction.

[mail transaction](http://tools.ietf.org/html/rfc5321#section-3.3)has finished at:

* Called <code>abort</code> callback.
* Call <code>reject</code> in milter.
* Call <code>temporary_failure</code> in milter.
* Call <code>discard</code> in milter.
* Call <code>accept</code> in milter.</dd>
<dt>finished</dt>
<dd>This method is called when completed milter protocol.TODO: write about timing</dd></dl>

## Using callbacks

We want to write the milter which will reject mails match againstspecified regular expression. The regular expression matches againstsubject and message body. It is necessary for us to select headercallback and body callback. Template is as following.

<pre>require 'milter/client'

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
end</pre>

## Check subject header

First, let's check subject header.

<pre>class MilterRegexp < Milter::ClientSession
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
end</pre>

Reject mails if header name is matched "subject" and its value matchesagainst specified regular expression.

## Operation check

Let's try to execute this milter.

Now, your milter is as following.

<pre>require 'milter/client'

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
end</pre>

You can execute this file as milter below command if you save thisfile as "milter-regexp.rb". We add "-v" option because it is easyto check operation.

<pre>% ruby milter-regexp.rb -v</pre>

In this case (default), milter run in foreground. You can checkoperation via other terminal.

milter-test-server is very useful to test milter.Milter which is written in Ruby is launched on "inet:20025@localhost".

<pre>% milter-test-server -s inet:20025
status: pass
elapsed-time: 0.00254348 seconds</pre>

You can see "status: pass" in your terminal if you can connect properly.Let's check another terminal.

<pre>[2010-08-01T05:44:34.157419Z]: [client][accept] 10:inet:55651@127.0.0.1
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
[2010-08-01T05:44:34.160578Z]: [sessions][finished] 1(+1) 0</pre>

You cannot connect to milter if you can see nothing in this terminal.Please check to launch milter or to specify correct address tomilter-test-server.

Let's check operation when milter process a mail included "viagra" in subject.You can reproduce the mail as following command.

<pre>% milter-test-server -s inet:20025 --header 'Subject:Buy viagra!!!'
status: reject
elapsed-time: 0.00144477 seconds</pre>

You can check expected result because you can see "status: reject" inyou terminal.

In another terminal, you can see log as followings.

<pre>...
[2010-08-01T05:49:49.275257Z]: [2] [command-decoder][header] <Subject>=<Buy viagra!!!>
[2010-08-01T05:49:49.275405Z]: [2] [client][reply][header][reject]
...</pre>

The mitler reject the mail when process subject header.

milter manager provides usuful tools and libraries.

## Check message body

Let's check message body.

<pre>class MilterRegexp < Milter::ClientSession
  def body(chunk)
    if @regexp =~ chunk
      reject
    end
  end
end</pre>

Reject mails if message body chunk matches against specified regularexpression.

Let's try. milter-test-server can specify message body via "--body"option.

<pre>% tool/milter-test-server -s inet:20025 --body 'Buy viagra!!!'
status: reject
elapsed-time: 0.00195496 seconds</pre>

It is expected result because you can see "status: reject" in yourterminal.

## Problems

There are some problems in this milter because this milter simplifyfor this tutorial.

1. Include MIME encoded header.      Decoded "=?ISO-2022-JP?B?GyRCJVAlJCUiJTAlaRsoQnZpYWdyYQ==?="      includes "viagra", but original header value does not match      against specified regular expression. And milter does not reject      this mail
2. The word splitted by chunk in message body.      For exapmle, Specified regular expression does not match if      first chunk has "via" and second one has "gra". And milter does      not reject this mail.

You can solve problems about header if you use NKF library as following.

<pre>require 'nkf'

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
end</pre>

You can solve problems about message body if milter check message bodywhen milter receives all chunks.

<pre>class MilterRegexp < Milter::ClientSession
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
end</pre>

You can test multiple chunks as following.

<pre>% milter-test-server -s inet:20025 --body 'Buy via' --body 'gra!!!'
status: reject
elapsed-time: 0.00379063 seconds</pre>

Mails are rejected if it includes multiple chunks.

However, in this case, all messages place on memory. This isperformance problem. In addition, this milter does not work if messagebody is BASE64 encoded.

[Mail](http://github.com/mikel/mail) is useful library thathandle mails.

## Conclusion

This document describes how to write milter in Ruby. It is easy towrite milter in Ruby. Let's try to write milter in Ruby!!


