---
title: CentOS 7へインストール
---

# CentOS 7へインストール --- CentOS 7へのmilter managerのインストール方法

## このドキュメントについて

CentOS 7に特化したmilter managerのインストール方法について説明します。CentOSに依存しない一般的なインストール情報はインストールを見てください。

CentOSのバージョンは7.2を想定しています。また、root権限での実行はsudoを使用することを想定しています。sudoを利用していない場合はsuなどroot権限で実行してください。

## パッケージのインストール

MTAは標準でインストールされているPostfixを利用することとします。

milterはspamass-milter、clamav-milter、milter-greylistを使用することとします。各milterはEPELにあるものを利用します。そのため、EPELを有効にします。

<pre>% sudo yum install -y epel-release</pre>

EPELを有効にしたらmilterをインストールします。

<pre>% sudo yum install -y spamass-milter-postfix clamav-scanner-systemd clamav-update clamav-milter clamav-milter-systemd milter-greylist</pre>

また、グラフ作成用にRRDtoolもインストールします。

<pre>% sudo yum install -y rrdtool</pre>

## milter managerパッケージのインストール

milter managerはyumでインストールできます。

まず、yumリポジトリを登録します。

<pre>% curl -s https://packagecloud.io/install/repositories/milter-manager/repos/script.rpm.sh | sudo bash</pre>

参考: https://packagecloud.io/milter-manager/repos/install

登録が完了したらmilter managerをインストールできます。

<pre>% sudo yum install -y milter-manager</pre>

## 設定

milterの基本的な設定方針は以下の通りです。

必要のない配送遅延をできるだけ抑えるため、milter-greylistは[S25R](http://gabacho.reto.jp/anti-spam/)にマッチするときのみ適用します。しかし、これはmilter-managerが自動で行うため、特に設定する必要はありません。

SELinux を有効にしたまま milter-manager を使用するためには多くの設定を変更する必要があるため、ここでは SELinux のポリシーモジュールのうちpostfix と milter を無効にして設定を進めることにします。

<pre>% sudo semodule -d postfix
% sudo semodule -d milter</pre>

### spamass-milterの設定

まず、spamdの設定をします。

デフォルトの設定ではスパム判定されたメールは件名に「[SPAM]」が追加されます。もし、これが嫌な場合は/etc/mail/spamassassin/local.cfを以下のように変更します。

変更前:

<pre>rewrite_header Subject [SPAM]</pre>

変更後:

<pre># rewrite_header Subject [SPAM]</pre>

また、スパム判定された場合のみ、その詳細をヘッダに追加するようにする場合は以下を追加します。

<pre>remove_header ham Status
remove_header ham Level</pre>

システム起動時にspamdを起動するようにします。

<pre>% sudo systemctl enable spamassassin</pre>

spamdを起動します。

<pre>% sudo systemctl start spamassassin</pre>

spamass-milterは以下のように設定します。

* 本文変更機能を無効にする。
* スコア15以上で拒否する。

/etc/sysconfig/spamass-milterを以下のように変更します。

変更前:

<pre>#EXTRA_FLAGS="-m -r 15"</pre>

変更後:

<pre>EXTRA_FLAGS="-m -r 15"</pre>

システム起動時にspamass-milterを起動するようにします。

<pre>% sudo systemctl enable spamass-milter</pre>

spamass-milterを起動します。

<pre>% sudo systemctl start spamass-milter</pre>

### clamav-milterの設定

ClamAVで使用するウィルス定義を自動更新する設定をします。

/etc/freshclam.confを次のように変更します。「Example」はコメントアウトし、「NotifyClamd」はコメントを外し値を変更しています。それ以外の項目はコメントを外しています。

変更前:

<pre>Example
#LogFacility LOG_MAIL
#NotifyClamd /path/to/clamd.conf</pre>

変更後:

<pre>#Example
LogFacility LOG_MAIL
NotifyClamd /etc/clamd.d/scan.conf</pre>

最初は手動でfreshclamを実行します。

<pre>% sudo freshclam</pre>

clamdの設定をします。

/etc/clamd.d/scan.confを次のように変更します。「Example」はコメントアウトし、それ以外の項目はコメントを外しています。

変更前:

<pre>Example
#LogFacility LOG_MAIL
#LocalSocket /run/clamd.scan/clamd.sock</pre>

変更後:

<pre>#Example
LogFacility LOG_MAIL
LocalSocket /run/clamd.scan/clamd.sock</pre>

システム起動時にclamdを起動するようにします。

<pre>% sudo systemctl enable clamd@scan</pre>

clamdを起動します。

<pre>% sudo systemctl start clamd@scan</pre>

clamav-milterの設定をします。

/etc/mail/clamav-milter.confを次のように変更します。「Example」はコメントアウトし、それ以外の項目はコメントを外しています。

変更前:

<pre>Example
#MilterSocket /run/clamav-milter/clamav-milter.socket
#MilterSocketMode 660
#ClamdSocket tcp:scanner.mydomain:7357
#LogFacility LOG_MAIL</pre>

変更後:

<pre>#Example
MilterSocket /run/clamav-milter/clamav-milter.socket
MilterSocketMode 660
ClamdSocket unix:/run/clamd.scan/clamd.sock
LogFacility LOG_MAIL</pre>

clamdのソケットにアクセスできるようにclamiltをclamscanグループに追加します。

<pre>% sudo usermod -G clamscan -a clamilt</pre>

システム起動時にclamav-milterを起動するようにします。

<pre>% sudo systemctl enable clamav-milter</pre>

clamav-milterを起動します。

<pre>% sudo systemctl start clamav-milter</pre>

### milter-greylistの設定

/etc/mail/greylist.confを編集し、以下のような設定にします。

* IPアドレスのマッチには前半24ビットのみを使う（送信元が複    数のMTAを利用している場合のGreylistの悪影響を抑えるため）
* 再送チェック時間を30分後（デフォルト）から10分後に短くす    る（Greylistの悪影響を抑えるため）
* オートホワイトリストの期間を1日（デフォルト）から1週間に    伸ばす（Greylistの悪影響を抑えるため）
* 信用するドメインの場合はSPFにパスしたらGreylistを使わない    （信用するドメインはmilter managerで指定する）
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

<pre>socket "/run/milter-greylist/milter-greylist.sock"
# ...
racl whitelist default</pre>

変更後:

<pre>socket "/run/milter-greylist/milter-greylist.sock" 660
# ...
subnetmatch /24
greylist 10m
autowhite 1w
sm_macro "trusted_domain" "{trusted_domain}" "yes"
racl whitelist sm_macro "trusted_domain" spf pass
racl greylist sm_macro "trusted_domain" not spf pass
racl greylist default</pre>

システム起動時にmilter-greylistを起動するようにします。

<pre>% sudo systemctl enable milter-greylist</pre>

milter-greylistを起動します。

<pre>% sudo systemctl start milter-greylist</pre>

### milter-managerの設定

まず、clamav-milterのソケットにアクセスできるようにmilter-managerをclamiltグループに加えます。

<pre>% sudo usermod -G clamilt -a milter-manager</pre>

同様に、milter-greylistのソケットにアクセスできるようにmilter-managerをmailグループとgrmilterグループに加えます。

<pre>% sudo usermod -G mail -a milter-manager
% sudo usermod -G grmilter -a milter-manager</pre>

同様に、spamass-milterのソケットにアクセスできるようにmilter-managerをpostfixグループに加えます。

<pre>% sudo usermod -G postfix -a milter-manager</pre>

milter-managerはシステムにインストールされているmilterを検出します。以下のコマンドでspamass-milter、clamav-milter、milter-greylistを検出していることを確認してください。

<pre>% sudo /usr/sbin/milter-manager -u milter-manager -g milter-manager --show-config</pre>

以下のように表示されていれば検出は成功しています。

<pre>...
define_milter("milter-greylist") do |milter|
  milter.connection_spec = "unix:/run/milter-greylist/milter-greylist.sock"
  ...
  milter.enabled = true
  ...
end
...
define_milter("clamav-milter") do |milter|
  milter.connection_spec = "unix:/var/run/clamav-milter/clamav-milter.socket"
  ...
  milter.enabled = true
  ...
end
...
define_milter("spamass-milter") do |milter|
  milter.connection_spec = "unix:/run/spamass-milter/postfix/sock"
  ...
  milter.enabled = true
  ...
end
...</pre>

milterの名前、ソケットのパス、enabledがtrueになっていることを確認してください。異なっていた場合は、設定を参考に/etc/milter-manager/milter-manager.local.confで設定してください。ただ、できれば、設定を変更せずに使用できるようにしたいと考えています。もし、検出できなかった環境のことを教えてもらえれば、milter-manager.local.confで設定しなくとも使用できるように検出方法を改良することができるかもしれません。

これでmilter-managerの設定は完了です。

システム起動時にmilter-managerを起動するようにします。

<pre>% sudo systemctl enable milter-manager</pre>

milter-managerを起動します。

<pre>% sudo systemctl start milter-manager</pre>

milter-test-serverで起動の確認をすることができます。

<pre>% sudo -u milter-manager -H milter-test-server -s unix:/var/run/milter-manager/milter-manager.sock</pre>

このように表示されれば成功です。

<pre>status: accept
elapsed-time: 0.128 seconds</pre>

起動に失敗しているときはこのように表示されます。

<pre>Failed to connect to unix:/var/run/milter-manager/milter-manager.sock</pre>

失敗している時はログを頼りに問題を解決します。--verboseオプションをつけると詳細なログが表示されます。また、デーモンプロセスにしないことにより、標準出力にもログが表示されます。

/etc/sysconfig/milter-managerに以下の内容を追加します。これにより、標準出力に詳細なログが表示されます。

<pre>OPTION_ARGS="--verbose --no-daemon"</pre>

milter-managerをもう一度起動します。

<pre>% sudo systemctl restart milter-manager</pre>

問題があればログが表示されます。起動しているmilter-managerはCtrl+cで終了することができます。

問題が解決した後は、/etc/sysconfig/milter-managerに追加したOPTION_ARGSをコメントアウトし、デーモンプロセスで起動するように戻してから、milter-managerを起動しなおしてください。

### Postfixの設定

Postfixを有効にします。

<pre>% sudo systemctl enable postfix
% sudo systemctl start postfix</pre>

次に、milterの設定をします。

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

/etc/postfix/main.cfに以下を追加します。

<pre>smtpd_milters = unix:/var/run/milter-manager/milter-manager.sock</pre>

Postfixの設定を再読み込みします。

<pre>% sudo systemctl reload postfix</pre>

以上で設定は完了です。

milter-managerはいくつかsyslogにログを出力します。mail.infoに出力するので、正常に動作していればmilter-managerからのログが/var/log/maillogにログがでるはずです。テストメールを送信して確認してください。

## まとめ

milter-managerを導入することにより、milterとPostfixを連携させる手間が削減されています。

通常であれば、/etc/postfix/main.cfにspamass-milter、clamav-milter、miler-greylistのそれぞれのソケットを指定する必要があります。しかし、milter-managerを導入することにより、milter-managerのソケットのみを指定するだけですむようになりました。各milterのソケットはmilter-managerが検出するため、typoなどの小さいですが気づきにくいミスに惑わされることがなくなります。

また、ここでは触れませんでしたが、milter-managerはsystemctl enableされているかどうかも検出します。そのため、milterを無効にしたい場合は、他のサービスと同様に以下のような手順になります。

<pre>% sudo systemctl stop milter-greylist
% sudo systemctl disable milter-greylist</pre>

milterを無効にしたら、milter-managerの設定を再読み込みします。milter-managerはmilterが無効になったことを検出し、無効になったmilterとは接続しなくなります。

<pre>% sudo systemctl reload milter-manager</pre>

Postfixの/etc/postfix/main.cfを変更する必要はありません。

CentOS上でmilterを複数使っている場合は、milter-managerを導入して、管理コストを削減することができます。

milter managerは運用を支援するツールも提供しています。インストールは必須ではありませんが、それらを導入することで運用コストを削減することができます。それらのツールもインストールする場合はCentOSへインストール（任意）を参照してください。


