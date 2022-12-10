---
title: milter-performance-check / milter manager / milter managerのマニュアル
---

# milter-performance-check / milter manager / milter managerのマニュアル

## 名前

milter-performance-check - MTAのパフォーマンスを計るプログラム

## 書式

<code>milter-performance-check</code> [<em>オプション ...</em>]

## 説明

milter-performance-checkはMTAの性能を計測するSMTPクライアントです。milter-test-serverでmilterのみの性能を計測し、milter-performance-checkでMTAとmilterを合わせた性能を計測するという住み分けです。

同様のツールにはPostfix付属のsmtp-sourceがあります。どちらも、同時に複数のSMTPセッションを張って一斉にメールを送信することができます。機能的にはsmtp-sourceの方が高機能です。

milter-performance-checkが便利なのはSMTPセッションの時間のみを計測してくれることです。smtp-sourceではtimeコマンドと組み合わせるなどして、smtp-source全体の実行時間を計測します。

実際は、SMTPセッションの時間のみでも、ツール全体の実行時間でもそれほど違いはでないと思います。また、テスト用のメール総数を多くすればするほど、SMTPセッションにかかる時間が大きくなり、ツール自体の実行時間による影響は小さくなります。

milter-performance-checkが提供している機能で十分な時は、milter-performance-checkを利用し、それでは不十分な時はsmtp-sourceを利用するとよいでしょう。

## オプション

<dl>
<dt>--help</dt>
<dd>利用できるオプションを表示して終了します。</dd>
<dt>--smtp-server=SERVER</dt>
<dd>接続先のSMTPサーバを指定します。

既定値はlocalhostです。</dd>
<dt>--smtp-port=PORT</dt>
<dd>接続先のSMTPサーバのポート番号指定します。

既定値は25です。</dd>
<dt>--connect-host=HOST</dt>
<dd>接続元のホスト名を指定します。

[PostfixのXCLIENT SMTP拡張](http://www.postfix-jp.info/trans-2.3/jhtml/XCLIENT_README.html)のNAMEを利用するので[smtpd_authorized_xclient_hosts](http://www.postfix-jp.info/trans-2.3/jhtml/postconf.5.html#smtpd_authorized_xclient_hosts)を適切に設定する必要があります。</dd>
<dt>--connect-address=ADDRESS</dt>
<dd>接続元のアドレスを指定します。

[PostfixのXCLIENT SMTP拡張](http://www.postfix-jp.info/trans-2.3/jhtml/XCLIENT_README.html)のADDRを利用するので[smtpd_authorized_xclient_hosts](http://www.postfix-jp.info/trans-2.3/jhtml/postconf.5.html#smtpd_authorized_xclient_hosts)を適切に設定する必要があります。</dd>
<dt>--helo-fqdn=FQDN</dt>
<dd>HELOコマンドでFQDNを使います。

既定値はlocalhost.localdomainです。</dd>
<dt>--starttls=HOW</dt>
<dd>1.6.9から使用可能。

STARTTLSを使うかどうかを指定します。指定可能な<var>HOW</var>は以下の通りです。

<dl>
<dt><kbd>auto</kbd></dt>
<dd>MTAがSTARTTLSをサポートしている場合はSTARTTLSを使います。（既定値）</dd>
<dt><kbd>always</kbd></dt>
<dd>常にSTARTTLSを使います。</dd>
<dt><kbd>disable</kbd></dt>
<dd>STARTTLSを使いません。</dd></dl>

既定値は<kbd>auto</kbd>です。</dd>
<dt>--auth-user=USER</dt>
<dd>1.6.9から使用可能。

SMTP認証のユーザ名として<var>USER</var>を使います。

既定値はありません。</dd>
<dt>--auth-password=PASSWORD</dt>
<dd>1.6.9から使用可能。

SMTP認証のパスワードとして<var>PASSWORD</var>を使います。

既定値はありません。</dd>
<dt>--auth-mechanism=MECHANISM</dt>
<dd>1.6.9から使用可能。

SMTP認証の方法として<var>MECHANISM</var>を使います。指定可能な<var>MECHANISM</var>は以下の通りです。

<dl>
<dt><kbd>auto</kbd></dt>
<dd>MTAがサポートしている認証方式を検出してそれを使います。（既定値）</dd>
<dt><kbd>plain</kbd></dt>
<dd>常にPLAINを使用します。</dd>
<dt><kbd>login</kbd></dt>
<dd>常にLOGINを使用します。</dd>
<dt><kbd>cram_md5</kbd>または<kbd>cram-md5</kbd></dt>
<dd>常にCRAM-MD5を使用します。</dd></dl>

既定値は<kbd>auto</kbd>です。</dd>
<dt>--auth-map=FILE</dt>
<dd>1.6.9から使用可能。

接続するMTAのアドレス・ポート番号ごとにSMTP認証の設定を<var>FILE</var>から読み込みます。<var>FILE</var>の書式は以下のようになっており、[Postfixのsmtp_sasl_password_maps](http://www.postfix.org/postconf.5.html#smtp_sasl_password_maps)でも使えます。

<pre>SERVER1:PORT USER1:PASSWORD1
SERVER2:PORT USER2:PASSWORD2
...</pre>

「smtp.example.com」の「submissionポート」（587番ポート）宛てのメールは「send-user」ユーザ・「secret」パスワードを使うという設定は以下の通りです。

<pre>smtp.example.com:587 send-user:secret</pre>

既定値はありません。</dd>
<dt>--from=FROM</dt>
<dd>MAILコマンドのアドレスにFROMを使います。

既定値はfrom@example.comです。</dd>
<dt>--force-from=FROM</dt>
<dd>送信するメールファイルを指定した場合でも、ファイル中にあるFrom:の値ではなく、FROMをMAILコマンドのアドレスに使います。

既定値はありません。</dd>
<dt>--recipient=RECIPIENT</dt>
<dd>RCPTコマンドのアドレスにRECIPIENTを使います。複数の宛先を指定する場合は複数回このオプションを指定してください。

既定値は[to@example.com]です。</dd>
<dt>--force-recipient=RECIPIENT</dt>
<dd>送信するメールファイルを指定した場合でも、ファイル中にあるTo:の値ではなく、RECIPIENTをRCPTコマンドのアドレスに使います。複数の宛先を指定する場合は複数回このオプションを指定してください。

既定値はありません。</dd>
<dt>--n-mails=N</dt>
<dd>合計でN個のメールを送信します。複数のメールが同時に送信されます。同時に最大で何通送るかは--n-concurrent-connectionsで指定します。

既定値は100です。</dd>
<dt>--n-additional-lines=N</dt>
<dd>メールの本文にN行追加します。

既定値はありません。（追加しません。）</dd>
<dt>--n-concurrent-connections=N</dt>
<dd>同時に最大N接続でメールを送信します。

既定値は10です。</dd>
<dt>--period=PERIOD</dt>
<dd>PERIOD（単位は秒、分、時間のどれか）の間に指定されたメールを送信します。各メールは間隔内で平均的に送信します。単位を省略した場合は秒として扱われます

例（送信メール数を100とする）:

* --period=5    # 0.05秒間隔で送信（5 / 100）
* --period=50s  # 0.5秒間隔で送信（50 / 100）
* --period=10m  # 6秒間隔で送信（60 * 10 / 100）
* --period=0.5h # 18秒間隔で送信（60 * 60 * 0.5 / 100）

既定値はありません。</dd>
<dt>--interval=INTERVAL</dt>
<dd>INTERVAL（単位は秒、分、時間のどれか）間隔で指定されたメールを送信します。単位を省略した場合は秒として扱われます。

例:

* --interval=5    # 5秒間隔で送信
* --interval=0.5s # 0.5秒間隔で送信
* --interval=10m  # 10分間隔で送信
* --interval=0.5h # 30分間隔で送信

既定値はありません。</dd>
<dt>--flood, --flood[=PERIOD]</dt>
<dd>PERIOD（単位は秒、分、時間のどれか）の間、ずっと指定されたメールを送信します。PERIODを省略した場合はC-cで終了するまでずっと指定したメールを送信します。単位を省略した場合は秒として扱われます。

既定値はありません。</dd>
<dt>--shuffle, --no-shuffle</dt>
<dd>送信予定のメールを無作為に並び替えてから送信します。

既定値はfalseです。（並び替えません。）</dd>
<dt>--report-failure-responses, --no-report-failure-responses</dt>
<dd>SMTPサーバが返した失敗時のメッセージを一番最後に表示します。

既定値はfalseです。（表示しません。）</dd>
<dt>--report-periodically, --report-periodically=INTERVAL</dt>
<dd>INTERVAL（単位は秒、分、時間のどれか）間隔で統計情報を表示します。INTERVALを省略した場合は1s（1秒）を指定したと扱われます。単位を省略した場合は秒として扱われます。

既定値はありません。（定期的に統計情報を表示しません。）</dd>
<dt>--reading-timeout=SECONDS</dt>
<dd>SMTPサーバからのレスポンスを待つ時間を指定します。<var>SECONDS</var>秒経ってもレスポンスが返ってこない場合はエラーになります。

既定値は60秒です。</dd></dl>

## 終了ステータス

常に0。

## 例

以下の例では、milter-performance-checkはlocalhostの25番ポートで動いているSMTPサーバに接続し、100通のメールを送ります。それぞれのメールの差出人はfrom@example.comで、宛先はwebmaster@localhostとinfo@localhostです。

<pre>% milter-performance-check --recipient=webmaster@localhost --recipient=info@localhost</pre>

以下の例では、milter-performance-checkは192.168.1.29の25番ポートで動いているSMTPサーバに接続し、/tmp/test-mails/以下にあるファイルをメールとして送信します。ファイルはRFC 2822のメールフォーマットでなければいけません。各メールは3秒毎（60 * 10 /100）にuser@localhost宛に送られます。--n-mails=1オプションが指定されているので、それぞれのメールは1通のみ送られます。

<pre>% milter-performance-check --n-mails=1 --smtp-server=192.168.1.102 --force-recipient=user@localhost --period=5m /tmp/test-mails/</pre>

## 関連項目

milter-report-statistics.rd.ja(1)


