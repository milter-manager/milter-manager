---
title: Rubyでmilter開発
---

# Rubyでmilter開発 --- Rubyバインディングのチュートリアル

## このドキュメントについて

milter managerが提供するライブラリを用いてRubyでmilterを開発する方法を説明します。

milterプロトコルの説明は[milter.org](http://www.milter.org)の[開発者向けドキュメント](https://www.milter.org/developers)を参照してください。

## インストール

Rubyでmilterを開発する場合はconfigure時に--enable-ruby-milterオプションを指定します。Debian GNU/Linux、Ubuntu、CentOS用のパッケージでは専用のパッケージがあるのでそれをインストールします。

Debian GNU/Linux、Ubuntuの場合:

<pre>% sudo aptitude -V -D -y install ruby-milter-core ruby-milter-client ruby-milter-server</pre>

CentOSの場合:

<pre>% sudo yum install -y ruby-milter-core ruby-milter-client ruby-milter-server</pre>

パッケージがない環境では以下のようにconfigureに--enable-ruby-milterオプションを指定してください。

<pre>% ./configure --enable-ruby-milter</pre>

インストールが成功しているかは以下のコマンドで確認できます。

<pre>% ruby -r milter -e 'p Milter::VERSION'
[1, 8, 0]</pre>

バージョン情報が出力されればインストールは成功しています。

## 概要

Rubyで開発したmilterは以下のようになります。

<pre>require 'milter/client'

class Session < Milter::ClientSession
  def initialize(context)
    super(context)
    # 初期化処理
  end

  def connect(host, address)
    # ...
  end

  # その他のコールバック定義
end

command_line = Milter::Client::CommandLine.new
command_line.run do |client, _options|
  client.register(Session)
end</pre>

それでは、指定された正規表現を含むメールを拒否するmilterを作ってみましょう。

## コールバック

イベントが発生する毎にmilterのコールバックメソッドが呼び出されます。ほとんどのイベントには付加情報があります。イベントの付加情報の受け渡し方法は2種類あります。1つはコールバックの引数として渡される方法で、もう1つはマクロとして渡される方法です。マクロについては後述します。ここではコールバックの引数として渡される情報についてだけ説明します。

以下がコールバックメソッドとその引数の一覧です。一覧を見た後に、今回のmilterで必要なコールバックを選びます。

<dl>
<dt>connect(host, address)</dt>
<dd>SMTPクライアントがSMTPサーバに接続したときに呼ばれます。

<var>host</var>は接続してきたSMTPクライアントのホスト名で、<var>address</var>はアドレスです。

例えば、localhostから接続した場合は以下のようになります。

<dl>
<dt>host</dt>
<dd>"localhost"</dd>
<dt>address</dt>
<dd><code>inet:45875@[127.0.0.1]</code>を表している<code>Milter::SocketAddress::IPv4</code>オブジェクト。</dd></dl></dd>
<dt>helo(fqdn)</dt>
<dd>SMTPクライアントがHELOまたはEHLOコマンドを送ったときに呼ばれます。

<var>fqdn</var>はHELO/EHLOで報告したFQDNです。

例えば、「EHLO mail.example.com」とした場合は以下のようになります。

<dl>
<dt>fqdn</dt>
<dd>"mail.example.com"</dd></dl></dd>
<dt>envelope_from(from)</dt>
<dd>SMTPクライアントがMAILコマンドを送ったときに呼ばれます。

<var>from</var>はMAILで報告した送信元アドレスです。

例えば、「MAIL FROM: &lt;user@example.com&gt;」とした場合は以下のようになります。

<dl>
<dt>from</dt>
<dd>"&lt;user@example.com&gt;"</dd></dl></dd>
<dt>envelope_recipient(to)</dt>
<dd>SMTPクライアントがRCPTコマンドを送ったときに呼ばれます。複数回RCPTコマンドを送った場合は複数回呼ばれます。

<var>to</var>はRCPTで報告した送信先アドレスです。

例えば、「RCPT TO: &lt;user@example.com&gt;」とした場合は以下のようになります。

<dl>
<dt>to</dt>
<dd>"&lt;user@example.com&gt;"</dd></dl></dd>
<dt>data</dt>
<dd>SMTPクライアントがDATAコマンドを送ったときに呼ばれます。</dd>
<dt>header(name, value)</dt>
<dd>送信するメールの中にあるヘッダーの数だけ呼ばれます。

<var>name</var>はヘッダーの名前で、<var>value</var>は値です。

例えば、「Subject: Hello!」というヘッダーがあった場合は以下のようになります。

<dl>
<dt>name</dt>
<dd>"Subject"</dd>
<dt>value</dt>
<dd>"Hello!"</dd></dl></dd>
<dt>end_of_header</dt>
<dd>送信するメールのヘッダー部分が終わったら呼ばれます。</dd>
<dt>body(chunk)</dt>
<dd>送信するメールの本文が送られてきたら呼ばれます。本文が小さいときは1回だけ呼ばれますが、大きい場合はいくつかの塊に分割されて複数回呼ばれます。

<var>chunk</var>は分割された本文です。

例えば、本文が「Hi!」だけであれば、1回だけ呼ばれて、以下のような値になります。

<dl>
<dt>chunk</dt>
<dd>"Hi!"</dd></dl></dd>
<dt>end_of_message</dt>
<dd>SMTPクライアントがデータ終了を表す「&lt;CR&gt;&lt;LF&gt;.&lt;CR&gt;&lt;LF&gt;」を送ったときに呼ばれます。</dd>
<dt>abort(state)</dt>
<dd>SMTPのトランザクションがリセットされたときに呼ばれます。具体的にはend_of_messageの後や、SMTPコマンドのRSETが送られたときです。

<var>state</var>はabortが呼び出されたタイミングを表すオブジェクトです。</dd>
<dt>unknown(command)</dt>
<dd>milterプロトコルで定義されていないコマンドが与えられたときに呼ばれます。

<var>command</var>は与えられたコマンド名です。</dd>
<dt>reset</dt>
<dd>initializeのときとメールトランザクションが終了したときに呼ばれます。

[メールトランザクション](http://tools.ietf.org/html/rfc5321#section-3.3)が終了するのは以下のときです。

* <code>abort</code>が呼ばれるとき
* <code>reject</code>を呼んだ時
* <code>temporary_failure</code>を呼んだ時
* <code>discard</code>を呼んだ時
* <code>accept</code>を呼んだ時</dd>
<dt>finished</dt>
<dd>milterプロトコルの処理が完了したときに呼ばれます。TODO: 呼ばれるタイミングについて書く</dd></dl>

## 利用するコールバック

今回作るmilterは指定された正規表現を含むメールを拒否するmilterです。正規表現はSubjectまたはメッセージ本文にマッチさせることにします。とすると、必要になるコールバックはヘッダー毎に呼び出されるheaderとメッセージ本文毎に呼び出されるbodyです。雛形は以下のようになります。

<pre>require 'milter/client'

class MilterRegexp < Milter::ClientSession
  def initialize(context, regexp)
    super(context)
    @regexp = regexp
  end

  def header(name, value)
    # ... Subjectをチェック
  end

  def body(chunk)
    # chunkをチェック
  end
end

command_line = Milter::Client::CommandLine.new
command_line.run do |client, _options|
  # バイアグラを含むメールを拒否
  client.register(MilterRegexp, /viagra/i)
end</pre>

## Subjectのチェック

まず、Subjectをチェックしましょう。

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

ヘッダー名（name）がSubjectのときに、ヘッダーの値（value）が指定された正規表現（@regexp）にマッチしていれば拒否（reject）しています。自然に書けていますね。

## 動作確認

それでは、実際に動かして試してみましょう。

現在は、以下のようになっているはずです。

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
    # chunkをチェック
  end
end

command_line = Milter::Client::CommandLine.new
command_line.run do |client, _options|
  # バイアグラを含むメールを拒否
  client.register(MilterRegexp, /viagra/i)
end</pre>

この状態ですでにmilterとして実行可能です。milter-regexp.rbというファイル名で保存した場合、以下のように実行します。-vオプションは詳細なログを出力するためのオプションで、動作を確認しやすいようにつけています。

<pre>% ruby milter-regexp.rb -v</pre>

milterはデフォルトではフォアグラウンドで動作します。別の端末からアクセスして動作を確認しましょう。

milterのテストにはmilter-test-serverが便利です。Rubyで実装されたmilterはデフォルトで「inet:20025@localhost」で起動するので、そのアドレスに接続します。

<pre>% milter-test-server -s inet:20025
status: pass
elapsed-time: 0.00254348 seconds</pre>

正常に接続できた場合は以上のように「status: pass」と表示されます。milterを起動している端末も確認してみましょう。以下のように表示されているはずです。

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

何も出力されていない場合はそもそもmilterに接続できていません。milterが起動しているか、milter-test-serverに正しいアドレスを指定しているかを確認してください。

それでは、Subjectに「viagra」と含んだメールの場合の動作を確認しましょう。「--header 'Subject:Buy viagra!!!'」というオプションを指定することでそのようなメールの動作を再現します。

<pre>% milter-test-server -s inet:20025 --header 'Subject:Buy viagra!!!'
status: reject
elapsed-time: 0.00144477 seconds</pre>

「status: reject」とでているので、期待通り拒否していることが確認できます。

milterの端末の方にも以下のようなログがでているはずです。

<pre>...
[2010-08-01T05:49:49.275257Z]: [2] [command-decoder][header] <Subject>=<Buy viagra!!!>
[2010-08-01T05:49:49.275405Z]: [2] [client][reply][header][reject]
...</pre>

Subjectヘッダーのときにrejectしていることがわかります。

MTAなしでmilterをテストできるコマンドや詳細なログ出力など、milter managerはmilterの開発に便利なツール・ライブラリを提供しています。

## メッセージ本体のチェック

次にメッセージ本体をチェックしましょう。

<pre>class MilterRegexp < Milter::ClientSession
  def body(chunk)
    if @regexp =~ chunk
      reject
    end
  end
end</pre>

メッセージ本文の一部（chunk）が指定された正規表現（@regexp）にマッチしていれば拒否（reject）しています。こちらも自然に書けていますね。

試してみましょう。milter-test-serverは「--body」オプションでメッセージ本文を指定できます。

<pre>% tool/milter-test-server -s inet:20025 --body 'Buy viagra!!!'
status: reject
elapsed-time: 0.00195496 seconds</pre>

「status: reject」となっているので、期待通り動作しています。

## 問題点

このmilterは説明のために簡略化されているため、いくつか問題点があります。例えば、以下のようなメールに対しては期待通り動きません。

1. ヘッダーの値がMIMEエンコードされている場合。例えば、      「=?ISO-2022-JP?B?GyRCJVAlJCUiJTAlaRsoQnZpYWdyYQ==?=」は      デコードすると「バイアグラviagra」になるが、この場合は正      規表現にマッチしないため、拒否しない。
2. メッセージ本体で単語が複数のチャンクにまたがった場合。      例えば、1つめのチャンクで「via」がきて2つめのチャンク      で「gra」がきた場合は正規表現にマッチしないため、拒否      しない。

ヘッダーの値に関しては以下のようにNKFなどを使ってMIMEエンコードをデコードすれば解決できます。

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

メッセージ本体に関しては、メッセージ本文を全部受信した後にもチェックする方法があります。

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

複数のチャンクにわかれた状態をテストするためには以下のように複数回「--body」オプションを指定します。

<pre>% milter-test-server -s inet:20025 --body 'Buy via' --body 'gra!!!'
status: reject
elapsed-time: 0.00379063 seconds</pre>

このように複数のチャンクにわかれてしまった場合でも期待通りに動きます。

ただし、これではすべてのメッセージをメモリ上に置いてしまうなど、効率の問題があります。また、メッセージ本文がBASE64でエンコードされている場合も動作しないという問題があります。これらは、ストリームとして処理したり、Content-Typeヘッダーの値などを確認した上でメッセージ本文を処理したりする必要があります。

メールを解析するライブラリとして[Mail](http://github.com/mikel/mail)があるので、それを使うとよいでしょう。

## まとめ

Rubyでmilterを作る方法について、実際にmilterを作りながら説明しました。Rubyを使うと簡単にmilterを実装できるので、ぜひ使ってみてください。


