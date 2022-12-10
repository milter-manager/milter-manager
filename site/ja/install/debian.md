---
title: Debianへインストール
---

# Debianへインストール --- Debian GNU/Linuxへのmilter managerのインストール方法

## このドキュメントについて

Debian GNU/Linuxに特化したmilter managerのインストール方法について説明します。Debianに依存しない一般的なインストール情報はインストールを見てください。

## パッケージのインストール

milter managerのサイトで現在の安定版stretchと開発版buster用のパッケージを配布しています。

まず、以下の内容の/etc/apt/sources.list.d/milter-manager.listを作成します。

<pre>% curl -s https://packagecloud.io/install/repositories/milter-manager/repos/script.deb.sh | sudo bash</pre>

参考: https://packagecloud.io/milter-manager/repos/install

### sidの場合

<em>NOTE: packagecloud はsidをサポートしていません。</em>

/etc/apt/sources.list.d/milter-manager.list:

<pre>deb http://downloads.sourceforge.net/project/milter-manager/debian/stable unstable main
deb-src http://downloads.sourceforge.net/project/milter-manager/debian/stable unstable main</pre>

### インストール

milter managerをインストールします。

<pre>% sudo aptitude update
% sudo aptitude -V -D -y install milter-manager</pre>

MTAはPostfixを利用することとします。

<pre>% sudo aptitude -V -D -y install postfix</pre>

milterはspamass-milter、clamav-milter、milter-greylistを使用することとします。

<pre>% sudo aptitude -V -D -y install spamass-milter clamav-milter milter-greylist</pre>

## 設定

milterの基本的な設定方針は以下の通りです。

UNIXドメインソケットで接続を受け付けるようにします。これは、セキュリティ面と速度面の理由からです。

UNIXドメインソケットはpostfixグループでの読み書き権限を設定します。これは、既存のmilterパッケージの設定をできるだけ利用するためです。

必要のない配送遅延をできるだけ抑えるため、milter-greylistは[S25R](http://gabacho.reto.jp/anti-spam/)にマッチするときのみ適用します。しかし、これはmilter-managerが自動で行うため、特に設定する必要はありません。

### spamass-milterの設定

まず、spamdの設定をします。

/etc/spamassassin/local.cfに以下の内容を追加します。これで、スパム判定された場合のみ、その詳細をヘッダに追加するようになります。

<pre>report_safe 0

remove_header ham Status
remove_header ham Level</pre>

spamdを有効にするため、/etc/default/spamassassinを以下のように変更します。

変更前:

<pre>ENABLED=0</pre>

変更後:

<pre>ENABLED=1</pre>

spamdを起動します。

<pre>% sudo /etc/init.d/spamassassin start</pre>

spamass-milterはデフォルトの設定で問題ありません。

### clamav-milterの設定

clamav-milterはデフォルトの設定で問題ありません。

### milter-greylistの設定

/etc/milter-greylist/greylist.confを編集し、以下のような設定にします。

* IPアドレスのマッチには前半24ビットのみを使う（送信元が複    数のMTAを利用している場合のGreylistの悪影響を抑えるため）
* 再送チェック時間を30分後（デフォルト）から10分後に短くす    る（Greylistの悪影響を抑えるため）
* オートホワイトリストの期間を1日（デフォルト）から1週間に    伸ばす（Greylistの悪影響を抑えるため）
* デフォルトでGreylistを使う

<pre># note
Greylistの悪影響を抑えるために制限を緩めているため、迷惑
メールが通過する確率がやや高くなりますが、誤判定時の悪影響を
抑える方を重視するべきです。Greylistですべての迷惑メールをブ
ロックしようとするのではなく、Greylistで検出できない迷惑メー
ルはSpamAssassinなど他の手法で対応します。milter managerはそ
のように複数のmilterを組み合わせた迷惑メール対策システムの構
築を支援します。</pre>

変更前:

<pre>racl whitelist default</pre>

変更後:

<pre>subnetmatch /24
greylist 10m
autowhite 1w
racl greylist default</pre>

次に、/etc/default/milter-greylistを変更し、milter-greylistを有効にします。milter-greylistの起動ファイルではソケットファイルをpostfixグループで作成できないため、IPv4で接続することとします。ただし、ローカルホストからの接続のみを受け付けることにします。

変更前:

<pre>ENABLED=0</pre>

変更後:

<pre>ENABLED=1
SOCKET="inet:11125@[127.0.0.1]"</pre>

milter-greylistを起動します。

<pre>% sudo /etc/init.d/milter-greylist start</pre>

### milter-managerの設定

milter-managerはシステムにインストールされているmilterを検出します。以下のコマンドでspamass-milter、clamav-milter、milter-greylistを検出していることを確認してください。

<pre>% sudo /usr/sbin/milter-manager -u milter-manager --show-config</pre>

以下のように表示されていれば検出は成功しています。

<pre>...
define_milter("milter-greylist") do |milter|
  milter.connection_spec = "inet:11125@[127.0.0.1]"
  ...
  milter.enabled = true
  ...
end
..
define_milter("clamav-milter") do |milter|
  milter.connection_spec = "unix:/var/run/clamav/clamav-milter.ctl"
  ...
  milter.enabled = true
  ...
end
..
define_milter("spamass-milter") do |milter|
  milter.connection_spec = "unix:/var/spool/postfix/spamass/spamass.sock"
  ...
  milter.enabled = true
  ...
end
..</pre>

milterの名前、ソケットのパス、enabledがtrueになっていることを確認してください。異なっていた場合は、設定を参考に/etc/milter-manager/milter-manager.confを編集してください。ただ、できれば、設定を変更せずに使用できるようにしたいと考えています。もし、検出できなかった環境のことを教えてもらえれば、milter-manager.confを編集しなくとも使用できるように検出方法を改良することができるかもしれません。

Postfixと一緒に動作するように/etc/default/milter-managerを編集します。

変更前:

<pre># For postfix, you might want these settings:
# SOCKET_GROUP=postfix
# CONNECTION_SPEC=unix:/var/spool/postfix/milter-manager/milter-manager.sock</pre>

変更後:

<pre># For postfix, you might want these settings:
SOCKET_GROUP=postfix
CONNECTION_SPEC=unix:/var/spool/postfix/milter-manager/milter-manager.sock</pre>

ソケットを作成するディレクトリーを作成します。

<pre>% sudo mkdir -p /var/spool/postfix/milter-manager/</pre>

milter-managerユーザーをpostfixグループに所属させます。

<pre>% sudo adduser milter-manager postfix</pre>

milter-managerの設定が完了したので、起動します。

<pre>% sudo /etc/init.d/milter-manager restart</pre>

milter-test-serverで起動の確認をすることができます。

<pre>% sudo -u postfix milter-test-server -s unix:/var/spool/postfix/milter-manager/milter-manager.sock</pre>

このように表示されれば成功です。

<pre>status: accept
elapsed-time: 0.128 seconds</pre>

起動に失敗しているときはこのように表示されます。

<pre>Failed to connect to unix:/var/spool/postfix/milter-manager/milter-manager.sock: No such file or directory</pre>

失敗している時はログを頼りに問題を解決します。--verboseオプションをつけると詳細なログが表示されます。また、デーモンプロセスにしないことにより、標準出力にもログが表示されます。

/etc/default/milter-managerに以下の内容を追加します。これにより、標準出力に詳細なログが表示されます。

<pre>OPTION_ARGS="--verbose --no-daemon"</pre>

milter-managerをもう一度起動します。

<pre>% sudo /etc/init.d/milter-manager restart</pre>

問題があればログが表示されます。起動しているmilter-managerはCtrl+cで終了することができます。

問題が解決した後は、/etc/default/milter-managerに追加したOPTION_ARGSをコメントアウトし、デーモンプロセスで起動するように戻してから、milter-managerを起動しなおしてください。

### Postfixの設定

まず、milterの設定をします。

/etc/postfix/main.cfに以下を追加します。

<pre>milter_protocol = 6
milter_default_action = accept
milter_mail_macros = {auth_author} {auth_type} {auth_authen}</pre>

それぞれ以下のような設定です。

<dl>
<dt>milter_protocol = 6</dt>
<dd>milterプロトコルバージョン6を使います。</dd>
<dt>milter_default_action = accept</dt>
<dd>milterに接続できないときはメールを受信します。milterとの通信に問題がある場合でもメールサーバの機能を停止させないためには、この設定が有効です。ただし、milterを復旧させるまでの間に、スパムメールやウィルスメールを受信してしまう可能性が高くなります。

迅速に復旧できる体制がととのっているのであれば、acceptではなく、tempfailを指定するのがよいでしょう。デフォルトではtempfailになっています。</dd>
<dt>milter_mail_macros = {auth_author} {auth_type} {auth_authen}</dt>
<dd>SMTP Auth関連の情報をmilterに渡します。milter-greylistなどが使用します。</dd></dl>

続いて、Postfixにmilter-managerを登録します。spamass-milter、clamav-milter、milter-greylistはmilter-manager経由で利用するので、Postfixにはmilter-managerだけを登録すればよいことに注意してください。

/etc/postfix/main.cfに以下を追加します。Postfixは/var/spool/postfix/にchrootすることに注意してください。

<pre>smtpd_milters = unix:/milter-manager/milter-manager.sock</pre>

Postfixの設定を再読み込みします。

<pre>% sudo /etc/init.d/postfix reload</pre>

以上で設定は完了です。

milter-managerはいくつかsyslogにログを出力します。mail.infoに出力するので、正常に動作していればmilter-managerからのログが/var/log/mail.infoにログがでるはずです。テストメールを送信して確認してください。

## まとめ

milter-managerを導入することにより、milterとPostfixを連携させる手間が削減されています。

通常であれば、Postfixのsmtpd_miltersにspamass-milter、clamav-milter、miler-greylistのソケットを指定する必要があります。しかし、milter-managerを導入することにより、milter-managerのソケットのみを指定するだけですむようになりました。各milterのソケットはmilter-managerが検出するため、typoなどの小さいですが気づきにくいミスに惑わされることがなくなります。

また、ここでは触れませんでしたが、milter-managerは/etc/default/milter-greylist内にあるようなENABLEDの設定も検出します。そのため、milterを無効にしたい場合は、以下のような手順になります。

<pre>% sudo /etc/init.d/milter-greylist stop
% sudo vim /etc/default/milter-greylist # ENABLED=1 => ENABLED=0</pre>

milterを無効にしたら、milter-managerの設定を再読み込みします。milter-managerはmilterが無効になったことを検出し、無効になったmilter とは接続しなくなります。

<pre>% sudo /etc/init.d/milter-manager reload</pre>

Postfixのmain.cfを変更する必要はありません。

Debian GNU/Linux上でmilterを複数使っている場合は、milter-managerを導入して、管理コストを削減することができます。

milter managerは運用を支援するツールも提供しています。インストールは必須ではありませんが、それらを導入することで運用コストを削減することができます。それらのツールもインストールする場合はDebianへインストール（任意）を参照してください。


