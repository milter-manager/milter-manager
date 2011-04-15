= ヘッダーに関するmilterの仕様

add_header/change_header/insert_header/delete_headerの実装か
らmilterの仕様を推測する。

== 互換性

残念ながら、SendmailとPostfixでは挙動が異なる。
milter managerはどっちとして動くのがよいだろうか。。。

それぞれのmilterごとにSendmailモードとか、Postfixモードとか
設定するのがよさそう？

== テストメールの送信方法

まず、テスト用SMPTセッションファイルを作成する。

header.smtp:
  EHLO localhost
  MAIL FROM:<root@localhost>
  RCPT TO:<root@localhost>
  DATA
  Subject: Hello
  From: <root@localhost>
  To: <root@localhost>
  X-Header1: Value1
  X-Header1: Value2
  X-Header1: Value3
  X-Header2: Value1
  X-Header2: Value2
  X-Header2: Value3

  World!
  .
  QUIT

SendmailまたはPostfixを起動する。

  % sudo service sendmail start
  % sudo service postfix start

テスト用SMTPセッションを実行。

  % netcat localhost smtp < header.smtp
  220 local.clear-code.com ESMTP Postfix (Debian/GNU)
  250-local.clear-code.com
  250-PIPELINING
  250-SIZE 10240000
  250-VRFY
  250-ETRN
  250-STARTTLS
  250-XCLIENT NAME ADDR PROTO HELO REVERSE_NAME PORT
  250-ENHANCEDSTATUSCODES
  250-8BITMIME
  250 DSN
  250 2.1.0 Ok
  250 2.1.5 Ok
  354 End data with <CR><LF>.<CR><LF>
  250 2.0.0 Ok: queued as 94351260266
  221 2.0.0 Bye
  %

↑はPostfixの場合。

配送されたメールを確認する。

  % sudo lv /var/mail/root

== 各コマンドの結果

=== add_header

以下のようなmilterを使う。

add-header.rb:
  require 'milter'

  class Session < Milter::ClientSession
    def end_of_message
      add_header("X-Header1", "Value Added1")
      add_header("X-Header1", "Value Added2")
      add_header("X-New-Header", "New Value1")
      add_header("X-New-Header", "New Value2")
    end
  end

  command_line = Milter::Client::CommandLine.new
  command_line.run do |client, _options|
    client.register(Session)
  end

SendmailとPostfixで違いがでるのは、同じヘッダー名のヘッダー
がすでに存在している場合である。Sendmailは同じ名前のヘッダー
があれば、そのヘッダーの直前に追加する。一方、Postfixは常に
一番最後に追加する。

Sendmailの場合:
  Return-Path: <root@local.clear-code.com>
  Received: from localhost (localhost [127.0.0.1])
          by local.clear-code.com (8.14.3/8.14.3/Debian-9.4) with ESMTP id p3F2ALj3000318
          for <root@localhost>; Fri, 15 Apr 2011 02:10:21 GMT
  Date: Fri, 15 Apr 2011 02:10:21 GMT
  Message-Id: <201104150210.p3F2ALj3000318@local.clear-code.com>
  Subject: Hello
  From: <root@local.clear-code.com>
  To: <root@local.clear-code.com>
  X-Header1: Value Added2
  X-Header1: Value Added1
  X-Header1: Value1
  X-Header1: Value2
  X-Header1: Value3
  X-Header2: Value1
  X-Header2: Value2
  X-Header2: Value3
  X-New-Header: New Value2
  X-New-Header: New Value1

  World!

Postfixの場合:
  Return-Path: <root@localhost>
  X-Original-To: root@localhost
  Delivered-To: root@localhost
  Received: from localhost (localhost [127.0.0.1])
          by local.clear-code.com (Postfix) with ESMTP id 2379226026C
          for <root@localhost>; Fri, 15 Apr 2011 11:11:02 +0900 (JST)
  Subject: Hello
  From: <root@localhost>
  To: <root@localhost>
  X-Header1: Value1
  X-Header1: Value2
  X-Header1: Value3
  X-Header2: Value1
  X-Header2: Value2
  X-Header2: Value3
  Message-Id: <20110415021102.2379226026C@local.clear-code.com>
  Date: Fri, 15 Apr 2011 11:11:02 +0900 (JST)
  X-Header1: Value Added1
  X-Header1: Value Added2
  X-New-Header: New Value1
  X-New-Header: New Value2

  World!
