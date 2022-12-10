---
title: FreeBSDへインストール
---

# FreeBSDへインストール --- FreeBSDへのmilter managerのインストール方法

## このドキュメントについて

FreeBSDに特化したmilter managerのインストール方法について説明します。FreeBSDに依存しない一般的なインストール情報はインストールを見てください。

FreeBSD 10.0-RELEASEを想定しています。

## パッケージのインストール

以下のパッケージをインストールすることにより、関連するパッケージもインストールされます。

MTAはPostfixを利用することとします。

<pre>% sudo pkg install --yes postfix</pre>

milterはspamass-milter、clamav-milter、milter-greylistを使用することとします。

<pre>% sudo pkg install --yes spamass-milter milter-greylist clamav-milter</pre>

milter-manager をインストールします。

<pre>% sudo pkg install --yes milter-manager</pre>

## 設定

milterの基本的な設定方針は以下の通りです。

UNIXドメインソケットで接続を受け付けるようにします。これは、セキュリティ面と速度面の理由からです。

milterの実効ユーザも一般ユーザにします。これもセキュリティ面の理由からです。UNIXドメインソケットはグループでの読み書き権限を設定します。読み書き権限のあるグループとしてmailグループを使用し、postfixユーザはmailグループに所属させます。

必要のない配送遅延をできるだけ抑えるため、milter-greylistは[S25R](http://gabacho.reto.jp/anti-spam/)にマッチするときのみ適用します。しかし、これはmilter-managerが自動で行うため、特に設定する必要はありません。

### spamass-milterの設定

まず、spamdの設定をします。

以下のような内容の/usr/local/etc/mail/spamassassin/local.cfを作成します。これで、スパム判定された場合のみ、その詳細をヘッダに追加するようになります。

<pre>remove_header ham Status
remove_header ham Level</pre>

spamdを有効にするため、/etc/rc.confに以下を追加します。

<pre>spamd_enable=YES</pre>

SMTPクライアントの同時接続数が多い場合は、最大同時接続数を増やします。デフォルトでは5接続になっています。迷惑メールを送る多くのSMTPクライアントはmilter-greylistでブロックされて、SpamAssassinまでは処理がまわってきません。まずは、SMTPクライアントの同時接続数の1/3程度にしておき、運用後に調整するとよいでしょう。例えば、同時に最大で約100接続ほどのSMTP接続がある場合は以下のように同時に30接続受け付けるようにするとよいでしょう。

<pre>spamd_flags="-c --max-children=30 "</pre>

運用後はmilter managerの統計情報をグラフ化し、調整してください。

Spamassassinのルールファイルを更新してからspamdを起動します。

<pre>% sudo sa-update
% sudo /usr/sbin/service sa-spamd start</pre>

次にspamass-milterの設定をします。spamass-milterはspamdユーザ・spamdグループで動かすことにします。

デフォルトではspamass-milterは/var/run/spamass-milter.sockにソケットファイルを作成します。しかし、/var/run/以下にはroot権限がないと新しくファイルを作成することができません。そのため、/var/run/spamass-milter/ディレクトリを作成し、そのディレクトリの所有者をspamdユーザにします。ソケットファイルはそのディレクトリ以下に作成することにします。

<pre>% sudo mkdir /var/run/spamass-milter/
% sudo /usr/sbin/chown spamd:spamd /var/run/spamass-milter</pre>

/etc/rc.confに以下を追加します。

<pre>spamass_milter_enable="YES"
spamass_milter_user="spamd"
spamass_milter_group="spamd"
spamass_milter_socket="/var/run/spamass-milter/spamass-milter.sock"
spamass_milter_socket_owner="spamd"
spamass_milter_socket_group="mail"
spamass_milter_socket_mode="660"
spamass_milter_localflags="-u spamd -- -u spamd"</pre>

spamass-milterを起動します。

<pre>% sudo /usr/sbin/service spamass-milter start</pre>

### clamav-milterの設定

まず、ClamAV本体の設定をします。

/etc/rc.confに以下を追加してclamdとfreshclamを有効にします。

<pre>clamav_clamd_enable="YES"
clamav_freshclam_enable="YES"</pre>

clamdを起動する前に最新の定義ファイルを取得します。

<pre>% sudo /usr/local/bin/freshclam</pre>

clamdとfreshclamを起動します。

<pre>% sudo /usr/sbin/service clamav-clamd start
% sudo /usr/sbin/service clamav-freshclam start</pre>

clamav-milterはデフォルトではclamavユーザ・clamavグループで起動します。一般ユーザなのでその設定を利用することにし、ソケットの書き込み権限のみを変更します。

/etc/rc.confに以下を追加します。

<pre>clamav_milter_enable="YES"
clamav_milter_socket_mode="660"
clamav_milter_socket_group="mail"</pre>

必要であれば/usr/local/etc/clamav-milter.confを変更します。例えば、以下のように変更します。

/usr/local/etc/clamav-milter.conf

変更前:

<pre>#OnInfected Quarantine
#AddHeader Replace
#LogSyslog yes
#LogFacility LOG_MAIL
#LogInfected Basic</pre>

変更後:

<pre>OnInfected Reject
AddHeader Replace
LogSyslog yes
LogFacility LOG_MAIL
LogInfected Full</pre>

それぞれ以下のような設定です。

<dl>
<dt>OnInfected Reject</dt>
<dd>ウィルスに感染したメールを受信拒否します。デフォルトではQuarantineでPostfixのholdキューに溜まりますが、定期的にholdキューを確認するのでなければ受信拒否した方がメンテナンスが楽になります。</dd>
<dt>AddHeader Replace</dt>
<dd>既存のX-Virus-Scannedヘッダがあっても上書きするようになります。この設定を有効にすることにより受信側でのウィルスチェックの結果がヘッダに入ることになります。通常、送信側より受信側の方のチェックの方が信頼できるはずなので（送信側が正しく報告しているかどうかは確かめられない）、この設定を有効にしておくとよいでしょう。</dd>
<dt>LogSyslog yes</dt>
<dd>syslogにログを出力します。負荷が大きい場合は無効にするとよいでしょう。</dd>
<dt>LogFacility LOG_MAIL</dt>
<dd>syslogのfacilityをLOG_MAILにして、/var/log/maillogにログを出力します。</dd>
<dt>LogInfected Full</dt>
<dd>ウィルスメールを見つけたときに詳細なログを出力します。</dd></dl>

clamav-milterを起動します。

<pre>% sudo /usr/sbin/service clamav-milter start</pre>

### milter-greylistの設定

milter-greylistはmailnullユーザ・mailグループで実行します。mailnullユーザはデフォルト設定ユーザであり、Postfixを使っている環境では使用されていないユーザなので、そのまま使うことにします。

/usr/local/etc/mail/greylist.conf.sampleを/usr/local/etc/mail/greylist.confにコピーし、以下のような設定にします。

* 実行グループをmailnullグループからmailグループにする
* ソケットファイルにはmailグループでも書き込めるようにする
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

<pre>socket "/var/milter-greylist/milter-greylist.sock"
user "mailnull:mailnull"
racl whitelist default</pre>

変更後:

<pre>socket "/var/milter-greylist/milter-greylist.sock" 660
user "mailnull:mail"
subnetmatch /24
greylist 10m
autowhite 1w
racl greylist default</pre>

/etc/rc.confに以下を追加します。

<pre>miltergreylist_enable="YES"
miltergreylist_runas="mailnull:mail"</pre>

milter-greylistを起動します。

<pre>% sudo /usr/sbin/service milter-greylist start</pre>

### milter-managerの設定

milter-managerはmilter-managerユーザで動作させるので、milter-managerユーザを作成します。

<pre>% sudo /usr/sbin/pw groupadd milter-manager
% sudo /usr/sbin/pw useradd milter-manager -g milter-manager -G mail -m</pre>

milter-managerはシステムにインストールされているmilterを検出します。以下のコマンドでspamass-milter、clamav-milter、milter-greylistを検出していることを確認してください。

<pre>% sudo /usr/local/sbin/milter-manager -u milter-manager --show-config</pre>

以下のように表示されていれば検出は成功しています。

<pre>...
define_milter("milter-greylist") do |milter|
  milter.connection_spec = "unix:/var/milter-greylist/milter-greylist.sock"
  ...
  milter.enabled = true
  ...
end
..
define_milter("clamav-milter") do |milter|
  milter.connection_spec = "unix:/var/run/clamav/clmilter.sock"
  ...
  milter.enabled = true
  ...
end
..
define_milter("spamass-milter") do |milter|
  milter.connection_spec = "unix:/var/run/spamass-milter/spamass-milter.sock"
  ...
  milter.enabled = true
  ...
end
..</pre>

milterの名前、ソケットのパス、enabledがtrueになっていることを確認してください。異なっていた場合は、設定を参考に/usr/local/etc/milter-manager/milter-manager.confを編集してください。ただ、できれば、設定を変更せずに使用できるようにしたいと考えています。もし、検出できなかった環境のことを教えてもらえれば、milter-manager.confを編集しなくとも使用できるように検出方法を改良することができるかもしれません。

FreeBSD上ではデフォルトでは/var/run/milter-manager/milter-manager.sockというソケットファイルを作成します。そのため、事前に/var/run/milter-manager/ディレクトリを作成しておく必要があります。

<pre>% sudo mkdir -p /var/run/milter-manager
% sudo /usr/sbin/chown -R milter-manager:milter-manager /var/run/milter-manager</pre>

milter-managerの設定が完了したので、起動の設定をします。

/etc/rc.confに以下を追加してmilter-managerを有効にします。

<pre>miltermanager_enable="YES"</pre>

milter-managerを起動します。

<pre>% sudo /usr/sbin/service milter-manager start</pre>

/usr/local/bin/milter-test-serverで起動の確認をすることができます。

<pre>% sudo -u milter-manager milter-test-server -s unix:/var/run/milter-manager/milter-manager.sock</pre>

このように表示されれば成功です。

<pre>status: pass
elapsed-time: 0.128 seconds</pre>

起動に失敗しているときはこのように表示されます。

<pre>Failed to connect to unix:/var/run/milter-manager/milter-manager.sock: No such file or directory</pre>

失敗している時はログを頼りに問題を解決します。--verboseオプションをつけると詳細なログが表示されます。また、デーモンプロセスにしないことにより、標準出力にもログが表示されます。

/etc/rc.confに以下を追加します。これにより、標準出力に詳細なログが表示されます。

<pre>miltermanager_debug="YES"</pre>

milter-managerをもう一度起動します。

<pre>% sudo /usr/sbin/service milter-manager restart</pre>

問題があればログが表示されます。起動しているmilter-managerはCtrl+cで終了することができます。

問題が解決した後は、/etc/rc.confに追加したmilter_manager_debugをコメントアウトし、デーモンプロセスで起動するように戻してから、milter-managerを起動しなおしてください。

### Postfixの設定

まず、postfixユーザをmailグループに追加します。

<pre>% sudo /usr/sbin/pw groupmod mail -m postfix</pre>

次に、milterの設定をします。

/usr/local/etc/postfix/main.cfに以下を追加します。

<pre>milter_protocol = 6
milter_default_action = accept
milter_mail_macros = {auth_author} {auth_type} {auth_authen}</pre>

それぞれ以下のような設定です。

<dl>
<dt>milter_protocol = 6</dt>
<dd>milterプロトコルのバージョン6を使います。</dd>
<dt>milter_default_action = accept</dt>
<dd>milterに接続できないときはメールを受信します。milterとの通信に問題がある場合でもメールサーバの機能を停止させないためには、この設定が有効です。ただし、milterを復旧させるまでの間に、スパムメールやウィルスメールを受信してしまう可能性が高くなります。

迅速に復旧できる体制がととのっているのであれば、acceptではなく、tempfailを指定するのがよいでしょう。デフォルトではtempfailになっています。</dd>
<dt>milter_mail_macros = {auth_author} {auth_type} {auth_authen}</dt>
<dd>SMTP Auth関連の情報をmilterに渡します。milter-greylistなどが使用します。</dd></dl>

続いて、Postfixにmilter-managerを登録します。spamass-milter、clamav-milter、milter-greylistはmilter-manager経由で利用するので、Postfixにはmilter-managerだけを登録すればよいことに注意してください。

/usr/local/etc/postfix/main.cfに以下を追加します。

<pre>smtpd_milters = unix:/var/run/milter-manager/milter-manager.sock</pre>

Postfixの設定を再読み込みします。

<pre>% sudo /usr/sbin/service postfix reload</pre>

以上で設定は完了です。

milter-managerはいくつかsyslogにログを出力します。mail.infoに出力するので、正常に動作していればmilter-managerからのログが/var/log/maillogにログがでるはずです。テストメールを送信して確認してください。

## まとめ

milter-managerを導入することにより、milterとPostfixを連携させる手間が削減されています。

通常であれば、Postfixのsmtpd_miltersにspamass-milter、clamav-milter、miler-greylistのソケットを指定する必要があります。しかし、milter-managerを導入することにより、milter-managerのソケットのみを指定するだけですむようになりました。各milterのソケットはmilter-managerが検出するため、typoなどの小さいですが気づきにくいミスに惑わされることがなくなります。

また、ここでは触れませんでしたが、milter-managerは/etc/rc.conf内のXXX_enabled="NO"の設定も検出します。そのため、milterを無効にしたい場合は、他のサーバプロセスと同様に以下のような手順になります。

<pre>% sudo /usr/sbin/service XXX stop
% sudo vim /etc/rc.conf # XXX_enabled="YES" => XXX_enabled="NO"</pre>

後は、milter-managerの設定を再読み込みすれば、milterが無効になったことを検出し、無効になったmilterとは接続しなくなります。

<pre>% sudo /usr/sbin/service milter-manager reload</pre>

Postfixのmain.cfを変更する必要はありません。

FreeBSD上でmilterを複数使っている場合は、milter-managerを導入して、管理コストを削減することができます。

milter managerは運用を支援するツールも提供しています。インストールは必須ではありませんが、それらを導入することで運用コストを削減することができます。それらのツールもインストールする場合はFreeBSDへインストール（任意）を参照してください。


