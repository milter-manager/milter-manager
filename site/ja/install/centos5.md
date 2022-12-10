---
title: CentOS 5へインストール
---

# CentOS 5へインストール --- CentOS 5へのmilter managerのインストール方法

## このドキュメントについて

CentOS 5に特化したmilter managerのインストール方法について説明します。CentOS 7に特化したmilter managerのインストール方法はCentOS 7へインストールを見てください。CentOS 6に特化したmilter managerのインストール方法はCentOS 6へインストールを見てください。CentOSに依存しない一般的なインストール情報はインストールを見てください。

CentOSのバージョンは5.8を想定しています。また、root権限での実行はsudoを使用することを想定しています。sudoを利用していない場合はsuなどroot権限で実行してください。

## パッケージのインストール

以下のパッケージをインストールすることにより、関連するパッケージもインストールされます。

<pre>% sudo yum install -y glib2 ruby</pre>

MTAは標準でインストールされているSendmailを利用することとします。

<pre>% sudo yum install -y sendmail-cf</pre>

milterはspamass-milter、clamav-milter、milter-greylistを使用することとします。各milterはRPMforgeにあるものを利用します。

32bit環境の場合は以下のようにRPMforgeを登録します。

<pre>% sudo rpm -Uhv http://packages.sw.be/rpmforge-release/rpmforge-release-0.5.2-2.el5.rf.i386.rpm</pre>

64bit環境の場合は以下のようにRPMforgeを登録します。

<pre>% sudo rpm -Uhv http://packages.sw.be/rpmforge-release/rpmforge-release-0.5.2-2.el5.rf.x86_64.rpm</pre>

RPMforgeを登録したらmilterをインストールします。

<pre>% sudo yum install -y spamass-milter clamav-milter milter-greylist</pre>

また、グラフ作成用にRRDtoolもインストールします。

<pre>% sudo yum install -y ruby-rrdtool</pre>

## milter managerパッケージのインストール

milter managerはyumでインストールできます。

まず、yumリポジトリを登録します。

<pre>% curl -s https://packagecloud.io/install/repositories/milter-manager/repos/script.rpm.sh | sudo bash</pre>

参考: https://packagecloud.io/milter-manager/repos/install

登録が完了したらmilter managerをインストールできます。

<pre>% sudo yum install -y milter-manager</pre>

## 設定

milterの基本的な設定方針は以下の通りです。

接続はIPv4ソケットでローカルホストからのみ接続を受け付けるようにします。

必要のない配送遅延をできるだけ抑えるため、milter-greylistは[S25R](http://gabacho.reto.jp/anti-spam/)にマッチするときのみ適用します。しかし、これはmilter-managerが自動で行うため、特に設定する必要はありません。

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

<pre>% sudo /sbin/chkconfig spamassassin on</pre>

spamdを起動します。

<pre>% sudo /sbin/service spamassassin start</pre>

spamass-milterは以下のように設定します。

* ソケットを変更する。
* 本文変更機能を無効にする。
* スコアが15を超えたら拒否する。

/etc/sysconfig/spamass-milterを以下のように変更します。

変更前:

<pre>#SOCKET=/var/run/spamass.sock
#EXTRA_FLAGS="-m -r 15"</pre>

変更後:

<pre>SOCKET="inet:11120@[127.0.0.1]"
EXTRA_FLAGS="-m -r 15"</pre>

システム起動時にspamass-milterを起動するようにします。

<pre>% sudo /sbin/chkconfig spamass-milter on</pre>

spamass-milterを起動します。

<pre>% sudo /sbin/service spamass-milter start</pre>

### clamav-milterの設定

clamdを起動します。

<pre>% sudo /sbin/service clamd start</pre>

clamav-milterのソケットを変更します。

/etc/clamav-milter.confを以下のように変更します。

変更前:

<pre>#MilterSocketMode 0660</pre>

変更後:

<pre>MilterSocketMode 0660</pre>

clamav-milterを起動します。

<pre>% sudo /sbin/service clamav-milter start</pre>

### milter-greylistの設定

/etc/mail/greylist.confを編集し、以下のような設定にします。

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

次に、以下の内容の/etc/sysconfig/milter-greylistを作成します。

<pre>OPTIONS="$OPTIONS -p inet:11122@[127.0.0.1]"</pre>

システム起動時にmilter-greylistを起動するようにします。

<pre>% sudo /sbin/chkconfig milter-greylist on</pre>

milter-greylistを起動します。

<pre>% sudo /sbin/service milter-greylist start</pre>

### milter-managerの設定

まず、clamav-milterのソケットにアクセスできるようにmilter-managerをclamavグループに加えます。

<pre>% sudo usermod -G clamav -a milter-manager</pre>

milter-managerはシステムにインストールされているmilterを検出します。以下のコマンドでspamass-milter、clamav-milter、milter-greylistを検出していることを確認してください。

<pre>% sudo /usr/sbin/milter-manager -u milter-manager --show-config</pre>

以下のように表示されていれば検出は成功しています。

<pre>...
define_milter("milter-greylist") do |milter|
  milter.connection_spec = "inet:11122@[127.0.0.1]"
  ...
  milter.enabled = true
  ...
end
...
define_milter("clamav-milter") do |milter|
  milter.connection_spec = "unix:/var/clamav/clmilter.socket"
  ...
  milter.enabled = true
  ...
end
...
define_milter("spamass-milter") do |milter|
  milter.connection_spec = "inet:11120@[127.0.0.1]"
  ...
  milter.enabled = true
  ...
end
...</pre>

milterの名前、ソケットのパス、enabledがtrueになっていることを確認してください。異なっていた場合は、設定を参考に/etc/milter-manager/milter-manager.confを編集してください。ただ、できれば、設定を変更せずに使用できるようにしたいと考えています。もし、検出できなかった環境のことを教えてもらえれば、milter-manager.confを編集しなくとも使用できるように検出方法を改良することができるかもしれません。

milter-managerの設定が完了したので、起動します。

<pre>% sudo /sbin/service milter-manager restart</pre>

milter-test-serverで起動の確認をすることができます。

<pre>% sudo -u milter-manager milter-test-server -s unix:/var/run/milter-manager/milter-manager.sock</pre>

このように表示されれば成功です。

<pre>status: pass
elapsed-time: 0.128 seconds</pre>

起動に失敗しているときはこのように表示されます。

<pre>Failed to connect to unix:/var/run/milter-manager/milter-manager.sock</pre>

失敗している時はログを頼りに問題を解決します。--verboseオプションをつけると詳細なログが表示されます。また、デーモンプロセスにしないことにより、標準出力にもログが表示されます。

/etc/sysconfig/milter-managerに以下の内容を追加します。これにより、標準出力に詳細なログが表示されます。

<pre>OPTION_ARGS="--verbose --no-daemon"</pre>

milter-managerをもう一度起動します。

<pre>% sudo /sbin/service milter-manager restart</pre>

問題があればログが表示されます。起動しているmilter-managerはCtrl+cで終了することができます。

問題が解決した後は、/etc/sysconfig/milter-managerに追加したOPTION_ARGSをコメントアウトし、デーモンプロセスで起動するように戻してから、milter-managerを起動しなおしてください。

### Sendmailの設定

まず、Sendmailを有効にします。

<pre>% sudo /sbin/chkconfig --add sendmail
% sudo /sbin/service sendmail start</pre>

Sendmailにmilter-managerを登録するために、/etc/mail/sendmail.mcに以下を追加します。

<pre>INPUT_MAIL_FILTER(`milter-manager', `S=local:/var/run/milter-manager/milter-manager.sock')</pre>

spamass-milter、clamav-milter、milter-greylistはmilter-manager経由で利用するので、Sendmailにはmilter-managerだけを登録すればよいことに注意してください。

Sendmailの設定を更新し、再読み込みします。

<pre>% sudo make -C /etc/mail
% sudo /sbin/service sendmail reload</pre>

以上で設定は完了です。

milter-managerはいくつかsyslogにログを出力します。mail.infoに出力するので、正常に動作していればmilter-managerからのログが/var/log/maillogにログがでるはずです。テストメールを送信して確認してください。

## まとめ

milter-managerを導入することにより、milterとSendmailを連携させる手間が削減されています。

通常であれば、sendmail.mcにspamass-milter、clamav-milter、miler-greylistのそれぞれのソケットを指定する必要があります。しかし、milter-managerを導入することにより、milter-managerのソケットのみを指定するだけですむようになりました。各milterのソケットはmilter-managerが検出するため、typoなどの小さいですが気づきにくいミスに惑わされることがなくなります。

また、ここでは触れませんでしたが、milter-managerは/sbin/chkconfig --addされているかどうかも検出します。そのため、milterを無効にしたい場合は、他のサービスと同様に以下のような手順になります。

<pre>% sudo /sbin/service milter-greylist stop
% sudo /sbin/chkconfig --del milter-greylist</pre>

milterを無効にしたら、milter-managerの設定を再読み込みします。milter-managerはmilterが無効になったことを検出し、無効になったmilter とは接続しなくなります。

<pre>% sudo /sbin/service milter-manager reload</pre>

Sendmailのsendmail.mcを変更する必要はありません。

CentOS上でmilterを複数使っている場合は、milter-managerを導入して、管理コストを削減することができます。

milter managerは運用を支援するツールも提供しています。インストールは必須ではありませんが、それらを導入することで運用コストを削減することができます。それらのツールもインストールする場合はCentOSへインストール（任意）を参照してください。


