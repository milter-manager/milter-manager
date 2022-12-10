---
title: milter-test-server / milter manager / milter managerのマニュアル
---

# milter-test-server / milter manager / milter managerのマニュアル

## 名前

milter-test-server - MTA側のmilterプロトコルを実装したプログラム

## 書式

<code>milter-test-server</code> [<em>オプション ...</em>]

## 説明

milter-test-serverはMTA側のmilterプロトコルを話します。つまり、MTA無しでmilterに接続することができます。現時点では、同様のツールは存在しないため、MTA+milterではなくmilter単体のテストを行うためには有用なツールです。例えば、以下のような用途に利用できます。

* milterの性能計測
* milterの動作確認

milter-test-serverは実行時間を表示するので、簡単な性能計測にも利用できます。このとき、MTAとは関係なくmilter単体での実行時間を確認できるため、MTA経由で試す場合よりも問題点の発見が容易になります。

milter-test-serverはヘッダや本文が変更された場合は、変更された内容を表示します。そのため、ヘッダや本文を変更するmilterのテストも容易に行うことができます。[C言語用テスティングフレームワークCutter](http://cutter.sourceforge.net/index.html.ja)などと組み合わせて自動化された単体テストを作成することもできます。

## オプション

<dl>
<dt>--help</dt>
<dd>利用できるオプションを表示して終了します。</dd>
<dt>--name=NAME</dt>
<dd>milter-test-serverの名前としてNAMEを使用します。名前は"{daemon_name}"マクロの値などで利用されます。

デフォルトはmilter-test-server（コマンドファイル名）です。</dd>
<dt>--connection-spec=SPEC</dt>
<dd>接続先のmilterのソケットを指定します。SPECの書式は次のうちのどれかです。

* unix:パス
* inet:ポート番号
* inet:ポート番号@ホスト名
* inet:ポート番号@[アドレス]
* inet6:ポート番号
* inet6:ポート番号@ホスト名
* inet6:ポート番号@[アドレス]

例:

* unix:/var/run/milter/milter-manager.sock
* inet:10025
* inet:10025@localhost
* inet:10025@[127.0.0.1]
* inet6:10025
* inet6:10025@localhost
* inet6:10025@[::1]</dd>
<dt>--negotiate-version=VERSION</dt>
<dd>クライアントへ送信するmilterプロトコルのバージョンとしてVERSIONを使用します。

デフォルトは8（Sendmail 8.14のデフォルトと同じ）です。</dd>
<dt>--connect-host=HOST</dt>
<dd>接続元のホスト名としてHOSTを使用します。

このホスト名はmilterのxxfi_connect()コールバックに渡ります。</dd>
<dt>--connect-address=SPEC</dt>
<dd>接続元のアドレスとしてSPECを使用します。SPECの書式は--connection-specの書式と同じです。

このアドレスはmilterのxxfi_connect()コールバックに渡ります。</dd>
<dt>--connect-macro=NAME:VALUE</dt>
<dd>xxfi_connect()コールバック時に送信するマクロを追加します。マクロの名前はNAMEで値はVALUEになります。複数のマクロを設定する場合はこのオプションを複数指定します。

xxfi_connect()コールバック時にclient_connectionsという名前で値が1のマクロを送信したい場合は以下のように指定します。

<pre>--connect-macro client_connections:1</pre></dd>
<dt>--helo-fqdn=FQDN</dt>
<dd>HELO/EHLO時の引数としてFQDNを使用します。

このfqdnはmilterのxxfi_helo()コールバックに渡ります。</dd>
<dt>--helo-macro=NAME:VALUE</dt>
<dd>xxfi_helo()コールバック時に送信するマクロを追加します。マクロの名前はNAMEで値はVALUEになります。複数のマクロを設定する場合はこのオプションを複数指定します。

xxfi_helo()コールバック時にclient_ptrという名前で値がunknownのマクロを送信したい場合は以下のように指定します。

<pre>--helo-macro client_ptr:unknown</pre></dd>
<dt>--envelope-from=FROM, -fFROM</dt>
<dd>MAIL FROM時の引数としてFROMを使用します。

このアドレスはmilterのxxfi_envfrom()コールバックに渡ります。</dd>
<dt>--envelope-from-macro=NAME:VALUE</dt>
<dd>xxfi_envfrom()コールバック時に送信するマクロを追加します。マクロの名前はNAMEで値はVALUEになります。複数のマクロを設定する場合はこのオプションを複数指定します。

xxfi_envfrom()コールバック時にclient_addrという名前で値が192.168.0.1のマクロを送信したい場合は以下のように指定します。

<pre>--envelope-from-macro client_addr:192.168.0.1</pre></dd>
<dt>--envelope-recipient=RECIPIENT, -rRECIPIENT</dt>
<dd>RCPT TO時の引数としてRECIPIENTを使用します。複数の宛先を指定したい場合は--envelope-recipientオプションを複数回指定してください。

このアドレスはmilterのxxfi_envrcpt()コールバックに渡ります。xxfi_envrcpt()は1つの宛先につき1回呼ばれるので、宛先が複数個ある場合はその分だけxxfi_envrcpt()が呼ばれます。</dd>
<dt>--envelope-recipient-macro=NAME:VALUE</dt>
<dd>xxfi_envrcpt()コールバック時に送信するマクロを追加します。マクロの名前はNAMEで値はVALUEになります。複数のマクロを設定する場合はこのオプションを複数指定します。

xxfi_envrcpt()コールバック時にclient_portという名前で値が2929のマクロを送信したい場合は以下のように指定します。

<pre>--envelope-recipient-macro client_port:2929</pre></dd>
<dt>--data-macro=NAME:VALUE</dt>
<dd>xxfi_data()コールバック時に送信するマクロを追加します。マクロの名前はNAMEで値はVALUEになります。複数のマクロを設定する場合はこのオプションを複数指定します。

xxfi_data()コールバック時にclient_nameという名前で値がunknownのマクロを送信したい場合は以下のように指定します。

<pre>--data-macro client_name:unknown</pre></dd>
<dt>--header=NAME:VALUE</dt>
<dd>名前がNAMEで値がVALUEのヘッダを追加します。複数のヘッダを追加したい場合は--headerオプションを複数回指定してください。

このヘッダはmilterのxxfi_header()コールバックに渡ります。xxfi_header()は1つのヘッダにつき1回呼ばれるので、ヘッダが複数個ある場合はその分だけxxfi_header()が呼ばれます。</dd>
<dt>--end-of-header-macro=NAME:VALUE</dt>
<dd>xxfi_eoh()コールバック時に送信するマクロを追加します。マクロの名前はNAMEで値はVALUEになります。複数のマクロを設定する場合はこのオプションを複数指定します。

xxfi_eoh()コールバック時にn_headersという名前で値が100のマクロを送信したい場合は以下のように指定します。

<pre>--end-of-header-macro n_headers:100</pre></dd>
<dt>--body=CHUNK</dt>
<dd>本文の一片としてCHUNKを追加します。複数のまとまりを追加する場合は--bodyオプションを複数回指定してください。

この本文のまとまりはmilterのxxfi_body()コールバックに渡ります。xxfi_body()は1つのまとまりにつき1回呼ばれるので、まとまりが複数個ある場合はその分だけxxfi_body()が呼ばれます。</dd>
<dt>--end-of-message-macro=NAME:VALUE</dt>
<dd>xxfi_eom()コールバック時に送信するマクロを追加します。マクロの名前はNAMEで値はVALUEになります。複数のマクロを設定する場合はこのオプションを複数指定します。

xxfi_eom()コールバック時にelapsedという名前で値が0.29のマクロを送信したい場合は以下のように指定します。

<pre>--end-of-message-macro elapsed:0.29</pre></dd>
<dt>--unknown=COMMAND</dt>
<dd>未知のSMTPコマンドとしてCOMMANDを使います。

このコマンドはmilterのxxfi_unknown()コールバックに渡ります。xxfi_unknown()コールバックは、xxfi_envrcpt()コールバックとxxfi_data()コールバックの間に呼ばれます。</dd>
<dt>--authenticated-name=NAME</dt>
<dd>SMTP Authで認証されたユーザ名を指定します。SASLでのログイン名に相当します。このオプションを指定するとMAIL FROMのときに<code>{auth_authen}</code>の値として<var>NAME</var>を渡します。</dd>
<dt>--authenticated-type=TYPE</dt>
<dd>SMTP Authの認証に使用した方法を指定します。SASLでのログインメソッドに相当します。このオプションを指定するとMAILFROMのときに<code>{auth_type}</code>の値として<var>TYPE</var>を渡します。</dd>
<dt>--authenticated-author=AUTHOR</dt>
<dd>SMTP Authで認証された送信者を指定します。SASL送信者に相当します。このオプションを指定するとMAIL FROMのときに<code>{auth_author}</code>の値として<var>AUTHOR</var>を渡します。</dd>
<dt>--mail-file=PATH</dt>
<dd>メールの内容として<var>PATH</var>にあるメールを使います。メール中にFrom:やTo:があった場合は、それらの値を送信元や宛先のアドレスとして使用します。</dd>
<dt>--output-message</dt>
<dd>milter適用後のメッセージを表示します。ヘッダを追加・削除したり、本文を置換するmilterの動作を確認したい場合はこのオプションを指定してください。</dd>
<dt>--color=[yes|true|no|false|auto]</dt>
<dd>--colorのみ、またはyes、trueを指定した場合はmilter適用後のメッセージを色付きで表示します。autoを指定した場合は、ターミナル上で実行していそうなら色を付けます。

既定値はnoです。色付けしません。</dd>
<dt>--connection-timeout=SECONDS</dt>
<dd>milterとの接続確立に使える時間を指定します。<var>SECONDS</var>秒以内に接続を確立できない場合はエラーになります。

既定値は300秒（5分）です。</dd>
<dt>--reading-timeout=SECONDS</dt>
<dd>milterからのレスポンスを待つ時間を指定します。<var>SECONDS</var>秒以内にレスポンスがこない場合はエラーになります。

既定値は10秒です。</dd>
<dt>--writing-timeout=SECONDS</dt>
<dd>milterへのコマンド送信に使える時間を指定します。<var>SECONDS</var>秒以内に送信できない場合はエラーになります。

既定値は10秒です。</dd>
<dt>--end-of-message-timeout=SECONDS</dt>
<dd>milterからのend-of-messageコマンドのレスポンスを待つ時間を指定します。<var>SECONDS</var>秒以内にレスポンスが完了しない場合はエラーになります。

既定値は300秒（5分）です。</dd>
<dt>--all-timeouts=SECONDS</dt>
<dd>--connection-timeout, --reading-timeout, --writing-timeout,--end-of-message-timeout の全てのタイムアウトの値を一括で設定します。</dd>
<dt>--threads=N</dt>
<dd>Nスレッドで同時にリクエストを送ります。

既定値は0で、メインスレッドのみでリクエストを送ります。</dd>
<dt>--verbose</dt>
<dd>実行時のログをより詳細に出力します。

「MILTER_LOG_LEVEL=all」というように環境変数を設定している場合と同じ効果があります。</dd>
<dt>--version</dt>
<dd>バージョンを表示して終了します。</dd></dl>

## 終了ステータス

milterセッションが始まった場合は0で、そうでない場合は0以外になります。ソケットの書式が間違っている場合かmilter-test-serverがmilterに接続できない場合は、milterセッションを始めることができません。

## 例

以下の例では、ホスト192.168.1.29上の10025番ポートで待ち受けているmilterに接続します。

<pre>% milter-test-server -s inet:10025@192.168.1.29</pre>

## 関連項目

milter-test-client.rd.ja(1),milter-performance-check.rd.ja(1)


