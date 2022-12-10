---
title: 設定
---

# 設定 --- milter-manager.confの書き方

## このドキュメントについて

milter-managerの設定ファイルmilter-manager.confの書き方について説明します。

## 場所

ここでは/usr/local/以下にインストールされたものとして説明します。configure時に--prefix=/usr/local/オプションを指定するか、--prefixオプションを省略した場合は/usr/local/以下にインストールされます。

この場合は、設定ファイルは/usr/local/etc/milter-manager/milter-manager.confになります。インストールが成功していれば、もうすでにファイルが存在するはずです。

## 概要

設定ファイルの先頭は以下のようになっています。

<pre># -*- ruby -*-

load("applicable-conditions/*.conf")
load_default
load_if_exist("milter-manager.local.conf")</pre>

通常は、この部分はそのままにしておき、milter-manager.confと同じディレクトリにmilter-manager.local.confというファイルを作成し、そのファイルに設定を記述します。

設定項目は以下のように分類されています。

* パッケージ関連
* セキュリティ関連
* ログ関連
* milter-manager関連
* コントローラ関連
* 子milter関連
* 組み込み適用条件
* 適用条件定義関連
* データベース関連

このうち、適用条件関連とデータベース関連はRubyの知識が必要になります。[S25R](http://gabacho.reto.jp/anti-spam/)などの有用な適用条件は標準で提供されているので、必ずしも自分で適用条件を定義できる必要はありません。そのため、適用条件の説明は一番最後にあります。適用条件を定義する必要がない場合は、適用条件関連の部分は読み飛ばしても構いません。

それぞれ順に説明する前に、設定をする上で便利なmilter-manager機能を紹介します。milter-managerを--show-configオプションを付きで起動すると、現在の設定内容が表示されます。

<pre>% /usr/local/sbin/milter-manager --show-config
package.platform = "debian"
package.options = nil

security.privilege_mode = false
security.effective_user = nil
security.effective_group = nil

log.level = "default"
log.path = nil
log.use_syslog = true
log.syslog_facility = "mail"

manager.connection_spec = nil
manager.unix_socket_mode = 0660
manager.unix_socket_group = nil
manager.remove_unix_socket_on_create = true
manager.remove_unix_socket_on_close = true
manager.daemon = false
manager.pid_file = nil
manager.maintenance_interval = 10
manager.suspend_time_on_unacceptable = 5
manager.max_connections = 0
manager.custom_configuration_directory = nil
manager.fallback_status = "accept"
manager.fallback_status_at_disconnect = "temporary-failure"
manager.event_loop_backend = "glib"
manager.n_workers = 0
manager.packet_buffer_size = 0
manager.connection_check_interval = 0
manager.chunk_size = 65535
manager.max_pending_finished_sessions = 0

controller.connection_spec = nil
controller.unix_socket_mode = 0660
controller.remove_unix_socket_on_create = true
controller.remove_unix_socket_on_close = true

define_applicable_condition("S25R") do |condition|
  condition.description = "Selective SMTP Rejection"
end

define_applicable_condition("Remote Network") do |condition|
  condition.description = "Check only remote network"
end</pre>

この内容で設定内容を確認することができます。

また、この書式はそのまま設定ファイルの書式になっているので、設定の書き方を忘れたときにはこの内容をヒントにすることができます。

それでは、それぞれの分類毎に説明します。

## パッケージ関連 {#package}

<dl>
<dt>package.platform</dt>
<dd><em>この項目は通常は変更する必要はありません。</em>

milterの自動検出方法はプラットフォーム毎に異なります。自動検出はmilterがプラットフォームで用いているパッケージシステムでインストールされていることを前提としています。そのため、実際のプラットフォームとmilter-managerが認識しているプラットフォームが異なると、自動検出がうまく動きません。

プラットフォームはビルド時に検出しています。検出結果が間違っている場合は、ビルド時に修正することができます。検出結果が間違っているが、ビルドをやり直すことができない場合に、この設定項目を利用します。

現在、標準で利用可能なプラットフォームは以下の通りです。

* debian: Debian GNU/LinuxやUbuntu LinuxなどDebian系Linux用
* redhat: CentOSなどRedHat系Linux
* freebsd: FreeBSD用
* pkgsrc: NetBSDやDragonFly BSDなどpkgsrcを利用している*BSD用

プラットフォーム名は「"」で囲んて"debian"というように指定します。

注意: 変更する場合はload_defaultの<em>前</em>に行って下さい。

例:

<pre>package.platform = "pkgsrc"</pre>

既定値:

<pre>package.platform = "debian" # 環境に依存</pre></dd>
<dt>package.options</dt>
<dd><em>この項目は通常は変更する必要はありません。</em>

package.platformと同様にビルド時に決定しています。

milter自動検出処理へ付加情報を渡すための項目です。"名前1=値1,名前2=値2,.."という形式で複数の情報を渡すことができます。

現在、この付加情報を使っているのはpkgsrcプラットフォームのときだけで、"prefix=rc.dがあるディレクトリのパス"という情報を使っています。例えば、/etc/rc.d/以下に起動スクリプトをインストールしている時は、"prefix=/etc"と指定します。

注意: 変更する場合はload_defaultの<em>前</em>に行って下さい。

例:

<pre>package.options = "prefix=/etc,name=value"</pre>

既定値:

<pre>package.options = nil # 環境に依存</pre></dd></dl>

## セキュリティ関連 {#security}

<dl>
<dt>security.privilege_mode</dt>
<dd>特権モードで動作するかどうかを指定します。子milter自動起動機能を利用する場合は有効にする必要があります。

有効にする場合はtrueを指定します。無効にする場合はfalseを指定します。

例:

<pre>security.privilege_mode = true</pre>

既定値:

<pre>security.privilege_mode = false</pre></dd>
<dt>security.effective_user</dt>
<dd>milter-managerプロセスの実効ユーザを指定します。ユーザを切り替えるにはmilter-managerコマンドをroot権限で起動する必要があります。

実効ユーザは「"」で囲んて"nobody"というように指定します。ユーザを指定しない場合はnilを指定します。

例:

<pre>security.effective_user = "nobody"</pre>

既定値:

<pre>security.effective_user = nil</pre></dd>
<dt>security.effective_group</dt>
<dd>milter-managerプロセスの実効グループを指定します。グループを切り替えるにはmilter-managerコマンドをroot権限で起動する必要があります。

実効グループは「"」で囲んで"nogroup"というように指定します。グループを指定しない場合はnilを指定します。

例:

<pre>security.effective_group = "nogroup"</pre>

既定値:

<pre>security.effective_group = nil</pre></dd></dl>

## ログ関連 {#log}

1.6.6から使用可能。

<dl>
<dt>log.level</dt>
<dd>ログレベルを指定します。ログレベルはすべて独立しているので、必要なログレベルを組み合わせて指定します。例えば、「infoとdebugとerrorレベルのログを出力する」というような指定になります

指定可能なログレベルは以下の通りです。

<dl>
<dt>default</dt>
<dd>critical、error、warnings、message、statisticsレベルのログを出力。既定値。</dd>
<dt>all</dt>
<dd>すべてのレベルのログを出力。</dd>
<dt>critical</dt>
<dd>致命的な問題のログを出力。</dd>
<dt>error</dt>
<dd>エラー時のログを出力。</dd>
<dt>warning</dt>
<dd>警告時のログを出力。</dd>
<dt>message</dt>
<dd>重要なメッセージを出力。</dd>
<dt>info</dt>
<dd>通常のメッセージを出力。</dd>
<dt>debug</dt>
<dd>デバッグログを出力。</dd>
<dt>statistics</dt>
<dd>統計用ログを出力。</dd>
<dt>profile</dt>
<dd>プロファイル用ログを出力。</dd></dl>

ログレベルは「"」で囲んで"all"というように指定します。複数のログレベルを指定する場合は"critical|error|warning"というように「|」で区切ります。

例:

<pre>log.level = "all"        # すべてのログを出力</pre>

既定値:

<pre>log.level = "default"</pre></dd>
<dt>log.path</dt>
<dd>ログの出力先のパスを指定します。

nilを指定すると標準出力に出力します。

例:

<pre>log.path = nil                            # 標準出力に出力
log.path = "/var/log/milter-manager.log"  # ファイルに出力</pre>

既定値:

<pre>log.path = nil</pre></dd>
<dt>log.use_syslog</dt>
<dd>syslogにもログを出力するかどうかを指定します。

出力する場合はtrueを指定します。出力しない場合はfalseを指定します。

例:

<pre>log.use_syslog = false   # syslogに出力しない</pre>

既定値:

<pre>log.use_syslog = true</pre></dd>
<dt>log.syslog_facility</dt>
<dd>syslog出力時に利用するファシリティを指定します。

指定可能なファシリティと対応するsyslogの定数は以下の通りです。

<dl>
<dt>authpriv</dt>
<dd>LOG_AUTHPRIV</dd>
<dt>cron</dt>
<dd>LOG_CRON</dd>
<dt>daemon</dt>
<dd>LOG_DAEMON</dd>
<dt>kern</dt>
<dd>LOG_KERN</dd>
<dt>local0</dt>
<dd>LOG_LOCAL0</dd>
<dt>local1</dt>
<dd>LOG_LOCAL1</dd>
<dt>local2</dt>
<dd>LOG_LOCAL2</dd>
<dt>local3</dt>
<dd>LOG_LOCAL3</dd>
<dt>local4</dt>
<dd>LOG_LOCAL4</dd>
<dt>local5</dt>
<dd>LOG_LOCAL5</dd>
<dt>local6</dt>
<dd>LOG_LOCAL6</dd>
<dt>local7</dt>
<dd>LOG_LOCAL7</dd>
<dt>lpr</dt>
<dd>LOG_LPR</dd>
<dt>mail</dt>
<dd>LOG_MAIL</dd>
<dt>news</dt>
<dd>LOG_NEWS</dd>
<dt>user</dt>
<dd>LOG_USER</dd>
<dt>uucp</dt>
<dd>LOG_UUCP</dd></dl>

ファシリティは「"」で囲んで"mail"というように指定します。

例:

<pre>log.syslog_facility = "local4"   # LOG_LOCAL4を使う</pre>

既定値:

<pre>log.syslog_facility = "mail"</pre></dd></dl>

## milter-manager関連 {#milter-manager}

<dl>
<dt>manager.connection_spec</dt>
<dd>milter-managerが接続を受け付けるソケットを指定します。

ソケットは「"」で囲んて"inet:10025"というように指定します。指定できる書式は以下の通りです。

* UNIXドメインソケット: unix:パス
  * 例: unix:/var/run/milter/milter-manager.sock
* IPv4ソケット: inet:ポート番号
  * 例: inet:10025
* IPv4ソケット: inet:ポート番号@ホスト名
  * 例: inet:10025@localhost
* IPv4ソケット: inet:ポート番号@[アドレス]
  * 例: inet:10025@[127.0.0.1]
* IPv6ソケット: inet6:ポート番号
  * 例: inet6:10025
* IPv6ソケット: inet6:ポート番号@ホスト名
  * 例: inet6:10025@localhost
* IPv6ソケット: inet6:ポート番号@[アドレス]
  * 例: inet6:10025@[::1]

もし、security.effective_user, security.effective_groupを指定している場合は、その権限でUNIXドメインソケットを作成します。ソケットを作成するディレクトリのパーミッションに注意してください。

IPv4ソケット・IPv6ソケットでホスト名を省略した場合は、すべてのネットワークインターフェイスから接続を受け付けます。ホスト名やアドレスを指定した場合はそのアドレスからのみ接続を受け付けます。

例:

<pre>manager.connection_spec = "unix:/var/run/milter/milter-manager.sock"</pre>

既定値:

<pre>manager.connection_spec = "inet:10025@[127.0.0.1]"</pre></dd>
<dt>manager.unix_socket_mode</dt>
<dd>milter-managerが接続を受け付けるUNIXドメインソケットのパーミッションを指定します。manager.connection_specでUNIXドメインソケットを使用している場合のみ利用されます。

8進数で値を指定するために、先頭に「0」をつけることを忘れないでください。

例:

<pre>manager.unix_socket_mode = 0600</pre>

既定値:

<pre>manager.unix_socket_mode = 0660</pre></dd>
<dt>manager.unix_socket_group</dt>
<dd>milter-managerが接続を受け付けるUNIXドメインソケットのグループを指定します。manager.connection_specでUNIXドメインソケットを使用している場合のみ利用されます。

ソケットのグループはsecurity.effective_user/security.effective_group権限で作成された後に、chown(2)で変更します。そのため、指定するグループはsecurity.effective_userの補助グループである必要があります。

グループは「"」で囲んて"nogroup"というように指定します。グループを指定しない場合はnilを指定します。

例:

<pre>manager.unix_socket_group = "nobody"</pre>

既定値:

<pre>manager.unix_socket_group = nil</pre></dd>
<dt>manager.remove_unix_socket_on_create</dt>
<dd>milter-managerが接続を受け付けるUNIXドメインソケットを作成する前にすでにファイルが存在した場合、削除するかどうかを指定します。manager.connection_specでUNIXドメインソケットを使用している場合のみ利用されます。

削除する場合はtrueを指定します。削除しない場合はfalseを指定します。

例:

<pre>manager.remove_unix_socket_on_create = false</pre>

既定値:

<pre>manager.remove_unix_socket_on_create = true</pre></dd>
<dt>manager.remove_unix_socket_on_close</dt>
<dd>milter-managerが終了するときに、接続を受け付けていたUNIXドメインソケットを削除するかどうかを指定します。manager.connection_specでUNIXドメインソケットを使用している場合のみ利用されます。

削除する場合はtrueを指定します。削除しない場合はfalseを指定します。

例:

<pre>manager.remove_unix_socket_on_close = false</pre>

既定値:

<pre>manager.remove_unix_socket_on_close = true</pre></dd>
<dt>manager.daemon</dt>
<dd>デーモンプロセスとして動作するかどうかを指定します。デーモンプロセスで動作する場合は、端末を切り離し、バックグラウンドで動作します。運用時はバックグラウンドで起動することをお勧めします。この設定項目はmilter-managerの--daemonコマンドラインオプションで上書きできるため、必ずしも設定ファイル内で設定する必要はありません。

デーモンプロセスとして動作する場合はtrueを指定します。そうでない場合はfalseを指定します。

例:

<pre>manager.daemon = true</pre>

既定値:

<pre>manager.daemon = false</pre></dd>
<dt>manager.pid_file</dt>
<dd>起動したmilter-managerのプロセスIDを保存するファイルを指定します。

もし、security.effective_user, security.effective_groupを指定している場合は、その権限でファイルへ書き込みます。ファイルのパーミッションに注意してください。

ファイルのパスは「"」で囲んで"/var/run/milter/milter-manager.pid"というように指定します。保存しない場合はnilを指定します。

例:

<pre>manager.pid_file = "/var/run/milter/milter-manager.pid"</pre>

既定値:

<pre>manager.pid_file = nil</pre></dd>
<dt>manager.maintenance_interval</dt>
<dd>何セッション終了毎にメンテナンス処理を実行するかを指定します。

現時点でのメンテナンス処理とはメモリ解放処理のことです。同時アクセス数が少ない環境では各セッション終了毎にメンテナンス処理を実行することによりメモリ使用量を抑えることができます。同時アクセス数が多い環境では複数セッション終了毎にまとめてメンテナンス処理を実行することにより処理効率をあげることができます。

0またはnilを指定した場合はメンテナンス処理を実行しません。

例:

<pre>manager.maintenance_interval = nil</pre>

既定値:

<pre>manager.maintenance_interval = 10</pre></dd>
<dt>manager.suspend_time_on_unacceptable</dt>
<dd>同時に多数の接続があり、MTAからの接続を受け付けることができないときに何秒待つかを指定します。ulimitやlimitで同時に開くことができるファイルディスクリプタ数を増やすことも検討してください。

例:

<pre>manager.suspend_time_on_unacceptable = 10</pre>

既定値:

<pre>manager.suspend_time_on_unacceptable = 5</pre></dd>
<dt>manager.max_connections</dt>
<dd>1.3.1から使用可能。

最大同時接続数を指定します。0を指定すると無制限になります。既定値では無制限です。

最大同時接続数の処理を行っているときに新しく接続要求があると、処理中の接続が終了するのを待ちます。処理中の接続が終了したかどうかはmanager.suspend_time_on_unacceptableで指定した秒数毎に確認します。

例:

<pre>manager.max_connections = 10 # 同時に最大10接続のみ受け付ける</pre>

既定値:

<pre>manager.max_connections = 0 # 制限無し</pre></dd>
<dt>manager.max_file_descriptors</dt>
<dd>1.3.1から使用可能。

プロセスが開くことができるファイルディスクリプタ数を指定します。0を指定するとシステムの既定値を変更しません。既定値は0なので、システムの既定値をそのまま使います。

milter-managerは1つのリクエストに対して「子milter数 +1（MTAとの接続用）」個のファイルディスクリプタを開きます。milter-manager内部でも数個のファイルディスクリプタを開くので少なくとも以下の個数のファイルディスクリプタが開けるようにしてください。

<pre>(子milter数 + 1) * 最大同時接続数 + 10（milter-manager内部で使用 + α）</pre>

プロセスが開くことができるファイルディスクリプタ数はsetrlimit(2)でソフトリミットとハードリミットを変更します。

例:

<pre>manager.max_file_descriptors = 65535</pre>

既定値:

<pre>manager.max_file_descriptors = 0</pre></dd>
<dt>manager.custom_configuration_directory</dt>
<dd>Webインターフェイスから変更した設定内容を保存するディレクトリを指定します。

ディレクトリのパスは「"」で囲んで"/tmp/milter-manager/"というように指定します。

nilを指定した場合は実効ユーザのホームディレクトリ直下に".milter-manager"というディレクトリを作成し、そのディレクトリを利用します。

例:

<pre>manager.custom_configuration_directory = "/tmp/milter-manager/"</pre>

既定値:

<pre>manager.custom_configuration_directory = nil</pre></dd>
<dt>manager.fallback_status</dt>
<dd>1.6.3から使用可能。

milter-manager内部で問題があったときにSMTPサーバへ返すステータスを指定します。milter-manager内部で問題が起こるのは以下のような場合です。

* 子milterが1つも登録されていない。
* メール本文用の一時ファイルを作成できない。
* など…

指定できる値は以下のいずれかの値です。

* "accept": メールを受信します。既定値です。
* "temporary-failure": メールを一時的に拒否します。
* "reject": メールを拒否します。
* "discard": メールを破棄します。

例:

<pre>manager.fallback_status = "reject"</pre>

既定値:

<pre>manager.fallback_status = "accept"</pre></dd>
<dt>manager.fallback_status_at_disconnect</dt>
<dd>1.6.3から使用可能。

SMTPクライアントがSMTPセッションの途中でSMTPサーバとの接続を切断したことを検出した時に返すステータスを指定します。切断の検出は既定値では無効になっているので、既定値ではこの設定が利用されることがありません。切断の検出を有効にする場合はmanager.use_netstat_connection_checkerを利用してください。

指定できる値は以下のいずれかの値です。

* "accept": メールを受信します。
* "temporary-failure": メールを一時的に拒否します。既定値です。
* "reject": メールを拒否します。
* "discard": メールを破棄します。

例:

<pre>manager.fallback_status_at_disconnect = "discard"</pre>

既定値:

<pre>manager.fallback_status_at_disconnect = "temporary-failure"</pre></dd>
<dt>manager.event_loop_backend</dt>
<dd><em>この項目は通常は使用する必要はありません。</em>

1.6.3から使用可能。

イベントループのバックエンドを指定します。100メール/秒以下のメール流量を処理するような中小規模のメールシステムではこの値を変更する必要はありません。100メール/秒以上のメール流量を処理するような大規模のメールシステムではこの値を"libev"に設定する必要があります。

注意: glibバックエンドは実装の都合上コールバックを使用します。この時、glibのコールバックの登録数の上限により通信回数が制限されることに注意してください。この制限事項はglibバックエンド対してのみあります。

指定できる値は以下のいずれかの値です。

* "glib": GLibのイベントループを使います。GLibのイベン       トループではI/O多重化にpoll(2)を使っています。既定値       です。
* "libev": I/O多重化に       [libev](http://libev.schmorp.de/)を使います。       システムによってepoll、kqueueまたはevent portsを使い       ます。

例:

<pre>manager.event_loop_backend = "libev"</pre>

既定値:

<pre>manager.event_loop_backend = "glib"</pre></dd>
<dt>manager.n_workers</dt>
<dd><em>この項目は通常は使用する必要はありません。</em>

1.6.3から使用可能。

メールを処理するプロセス数を指定します。100メール/秒以下のメール流量を処理するような中小規模のメールシステムや、非常に重いmilterを使用しないようなメールシステムではこの値を変更する必要はありません。非常に重いmilterを使用しながら100メール/秒以上のメール流量を処理するような大規模のメールシステムではこの値を増やす必要があります。

指定できる値は0以上、1000以下です。0のときはワーカープロセスを使用しません。

例:

<pre>manager.n_workers = 10</pre>

既定値:

<pre>manager.n_workers = 0 # ワーカープロセスを使用しない</pre></dd>
<dt>manager.packet_buffer_size</dt>
<dd><em>この項目は通常は使用する必要はありません。</em>

1.6.3から使用可能。

end-of-message時に送信パケットをバッファリングするためのバッファサイズを指定します。バッファリングしているパケットの量が指定したバイト以上になるとまとめてパケットを送信します。0を指定するとバッファリングしません。

end-of-message時にadd_headerやdelete_recipientなどメッセージ変更操作を多くする場合にパフォーマンスがよくなる可能性があります。通常はほとんど影響がありません。

例:

<pre>manager.packet_buffer_size = 4096 # 4KB溜まるまで送信しない。</pre>

既定値:

<pre>manager.packet_buffer_size = 0 # バッファリングを無効にする。</pre></dd>
<dt>manager.chunk_size</dt>
<dd><em>この項目は通常は使用する必要はありません。</em>

1.8.0から使用可能。

2番目以降の子milterにbodyパケットを送るときのデータサイズを指定します。最大値は65535バイトで、これが既定値です。データサイズを小さくすることにより通信のオーバーヘッドが少し増えますが、それでも1回あたりのデータサイズを小さくしたい場合のみ使ってください。

例:

<pre>manager.chunk_size = 4096 # 本文データを4KBずつ送る</pre>

既定値:

<pre>manager.chunk_size = 65535 # 本文データを64KBずつ送る</pre></dd>
<dt>manager.max_pending_finished_sessions</dt>
<dd><em>この項目は通常は使用する必要はありません。</em>

1.8.6から使用可能。

milter managerはスループットを向上させるため、スループットに影響しない処理は何も処理がないときまで処理を遅延します。終了したmilterセッションの後始末処理もそのような遅延される処理の1つです。

この項目を設定することで、他に処理があるときでもmilterセッションの後始末処理を行うようになります。通常は、複数のmilterセッションを処理している場合でも途中に他に何も処理するものがない状態になりますが、同時接続数が非常に多くなると常になんらかの処理を実施し続けるため、後始末処理が実行されなくなります。後始末処理ではソケットのクローズも行っているため、長い間、後始末処理が実行されないと開けるファイルディスクリプタ数が足りなくなる危険性があります。

常になんらかの処理を実行し続けている状態は過負荷の状態なので、本来であれば、そうならないようにmanager.n_workersを設定してワーカー数を増やした構成にすることが望ましいです。

この項目に0より大きい値を設定すると、他に処理があるときでも、指定した値分セッションを処理した後に後始末処理を実行します。もちろん、他に処理がないときも後始末処理を実行するので、通常時はスループットへの影響はありません。負荷が高くなった時のみこの値が影響します。

規定値は0でこの機能は無効になっています。

例:

<pre># セッションが終了したら毎回すぐに終了処理を行う
manager.max_pending_finished_sessions = 1</pre>

既定値:

<pre># なにも処理がないときのみセッションの終了処理を行う
manager.max_pending_finished_sessions = 0</pre></dd>
<dt>manager.use_netstat_connection_checker</dt>
<dd>1.5.0から使用可能。

<kbd>netstat</kbd>コマンドの出力を解析してSMTPクライアントがまだSMTPサーバと接続しているかを確認します。

この機能はmilter（milter-greylist）を用いた[taRgrey](http://k2net.hakuba.jp/targrey/)を実現しているときなど、SMTPクライアントが自発的に切断したことを検出して途中で処理を止めたい場合に有用です。この機能を用いることによりtaRgreyの問題点の1つであるSMTPサーバのプロセス数増大を抑えることができます。プロセス数が増大するとメモリ使用量が増えるため、プロセス数増大を抑えることはメモリ使用量を抑えることにつながります。

接続は5秒毎に確認します。この間隔は変更することも可能ですが、通常は変更する必要はありません。

例:

<pre>manager.use_netstat_connection_checker    # 5秒間隔で確認
manager.use_netstat_connection_checker(1) # 1秒間隔で確認</pre>

初期値:

<pre>確認しない</pre></dd>
<dt>manager.connection_check_interval</dt>
<dd><em>この項目は通常は直接使用する必要はありません。</em>

1.5.0から使用可能。

SMTPクライアントがまだSMTPサーバと接続しているかどうかを確認する間隔を秒単位で指定します。

0を指定すると確認しません。

どのようにして接続しているかどうかを確認するかはmanager.define_connection_checkerで定義します。

例:

<pre>manager.connection_check_interval = 5 # 5秒間隔で確認</pre>

既定値:

<pre>manager.connection_check_interval = 0</pre></dd>
<dt>manager.define_connection_checker(name) {|context| ... # -&gt; true/false}</dt>
<dd><em>この項目は通常は直接使用する必要はありません。</em>

1.5.0から使用可能。

manager.connection_check_intervalで指定した秒毎にSMTPクライアントがまだSMTPサーバと接続しているかを確認します。ブロックがtrueを返したときはまだ接続していることを示し、falseを返したときは接続が切れたことを示します。

<dl>
<dt>name</dt>
<dd>確認処理につける名前です。</dd>
<dt>context</dt>
<dd>ブロックに渡される現在の状況を知っているオブジェクトです。以下の情報を取得できます。</dd>
<dt>context.smtp_client_address</dt>
<dd>接続確認対象のSMTPクライアントのIPアドレスです。socket_addressと同じオブジェクトです。</dd>
<dt>context.smtp_server_address</dt>
<dd>SMTPクライアントの接続を受け付けたSMTPサーバ側のIPアドレスです。socket_addressと同じオブジェクトです。</dd></dl>

例:

<pre># ローカルネットワーク以外からの接続は強制的に切断したとみなす
manager.define_connection_checker("netstat-check") do |context|
  context.smtp_client_address.local?
end</pre></dd>
<dt>manager.report_memory_statistics</dt>
<dd>1.5.0から使用可能。

メンテナンス処理が実行される度にメモリ使用量をログに出力します。

現在は以下のようなフォーマットで出力されますが、変更される可能性があります。

<pre>Mar 28 15:16:58 mail milter-manager[19026]: [statistics] [maintain][memory] (28048KB) total:6979 Proc:44 GLib::Object:18</pre>

使用例:

<pre>manager.report_memory_statistics</pre></dd>
<dt>manager.maintained {...}</dt>
<dd><em>この項目は通常は直接使用する必要はありません。</em>

1.5.0から使用可能。

メンテナンス処理が実行される度に指定した処理を実行します。

以下の例はメンテナンス処理が実行する度にログを出力する設定です。

使用例:

<pre>manager.maintained do
  Milter::Logger.info("maintained!")
end</pre></dd>
<dt>manager.event_loop_created {|loop| ...}</dt>
<dd><em>この項目は通常は直接使用する必要はありません。</em>

1.6.8から使用可能。

イベントループが作成されたときに指定した処理を実行します。イベントループが作成されるのは初期化時のみです。時のみ指定した実行します。

以下の例は1秒ごとにログを出力するコールバックを登録する設定です。

使用例:

<pre>manager.event_loop_created do |loop|
  loop.add_timeout(1) do
    Milter::Logger.info("timeout!")
    true
  end
end</pre></dd></dl>

## コントローラ関連 {#controller}

<dl>
<dt>controller.connection_spec</dt>
<dd>milter-managerを制御するための接続を受け付けるソケットを指定します。

書式はmanager.connection_specと同じです。

例:

<pre>controller.connection_spec = "inet:10026@localhost"</pre>

既定値:

<pre>controller.connection_spec = nil</pre></dd>
<dt>controller.unix_socket_mode</dt>
<dd>milter-managerを制御するための接続を受け付けるUNIXドメインソケットのパーミッションを指定します。controller.connection_specでUNIXドメインソケットを使用している場合のみ利用されます。

8進数で値を指定するために、先頭に「0」をつけることを忘れないでください。

例:

<pre>controller.unix_socket_mode = 0600</pre>

既定値:

<pre>controller.unix_socket_mode = 0660</pre></dd>
<dt>controller.remove_unix_socket_on_create</dt>
<dd>milter-managerを制御するための接続を受け付けるUNIXドメインソケットを作成する前にすでにファイルが存在した場合、削除するかどうかを指定します。controller.connection_specでUNIXドメインソケットを使用している場合のみ利用されます。

削除する場合はtrueを指定します。削除しない場合はfalseを指定します。

例:

<pre>controller.remove_unix_socket_on_create = false</pre>

既定値:

<pre>controller.remove_unix_socket_on_create = true</pre></dd>
<dt>controller.remove_unix_socket_on_close</dt>
<dd>milter-managerが終了するときに、milter-managerを制御するための接続を受け付けていたUNIXドメインソケットを削除するかどうかを指定します。controller.connection_specでUNIXドメインソケットを使用している場合のみ利用されます。

削除する場合はtrueを指定します。削除しない場合はfalseを指定します。

例:

<pre>controller.remove_unix_socket_on_close = false</pre>

既定値:

<pre>controller.remove_unix_socket_on_close = true</pre></dd></dl>

## 子milter関連 {#child-milter}

子milterに関連する設定項目について説明します。

### 子milter定義

子milterは以下の書式で登録します。

<pre>define_milter("名前") do |milter|
  milter.XXX = ...
  milter.YYY = ...
  milter.ZZZ = ...
end</pre>

例えば、「inet:10026@localhost」で接続待ちしているmilterを「test-milter」という名前で登録する場合は以下のようになります。

<pre>define_milter("test-milter") do |milter|
  milter.connection_spec = "inet:10026@localhost"
end</pre>

define_milter do ... end内で設定できる項目は以下の通りです。

必須の項目はmilter.connection_specだけです。

<dl>
<dt>milter.connection_spec</dt>
<dd>子milterが接続待ちしているソケットを指定します。<em>必須項目</em>です。

書式はmanager.connection_specと同じです。

例:

<pre>milter.connection_spec = "inet:10026@localhost"</pre>

既定値:

<pre>milter.connection_spec = nil</pre></dd>
<dt>milter.description</dt>
<dd>子milterの説明を指定します。

説明は「"」で囲んで"test milter"というように指定します。

例:

<pre>milter.description = "test milter"</pre>

既定値:

<pre>milter.description = nil</pre></dd>
<dt>milter.enabled</dt>
<dd>子milterを利用するかどうかを指定します。

利用する場合はtrueを指定します。利用しない場合はfalseを指定します。

例:

<pre>milter.enabled = false</pre>

既定値:

<pre>milter.enabled = true</pre></dd>
<dt>milter.fallback_status</dt>
<dd>子milterに問題があったとき、指定したステータスを返したとして扱います。

指定できる値は以下のいずれかの値です。

* "accept": メールを受信します。既定値です。
* "temporary-failure": メールを一時的に拒否します。
* "reject": メールを拒否します。
* "discard": メールを破棄します。

例:

<pre>milter.fallback_status = "temporary-failure"</pre>

既定値:

<pre>milter.fallback_status = "accept"</pre></dd>
<dt>milter.evaluation_mode</dt>
<dd>1.3.1から使用可能。

評価モードにするかどうかを指定します。評価モードでは子milterの結果をMTAに返さないので、既存のメールシステムには影響を与えません。

評価モードでも統計用のログが出力されるため、本来なら子milterがMTAにどのような結果を返していたかを視覚化できます。

評価モードにする場合はtrueを指定します。

false（既定値）の場合は、子milterがrejectを返すとMTAにrejectと返し、処理が終了します。trueの場合は子milterがrejectを返してもMTAにはrejectを返さず、処理が継続します。継続している処理の中では「子milterがrejectを返した」という情報を利用できます。その情報を利用して適用条件を記述することができます。

例:

<pre>milter.evaluation_mode = true</pre>

既定値:

<pre>milter.evaluation_mode = false</pre></dd>
<dt>milter.applicable_conditions</dt>
<dd>子milterを適用する条件を指定します。条件は複数指定できます。1つでも条件を満たさない場合は子milterの適用は中止されます。

利用可能は適用条件は以下のコマンドで確認できます。

<pre>% /usr/local/sbin/milter-manager --show-config | grep define_applicable_condition
define_applicable_condition("S25R") do |condition|
define_applicable_condition("Remote Network") do |condition|</pre>

この場合は"S25R"と"Remote Network"が利用可能です。

適用条件は標準で提供されているだけではなく、独自に定義することもできます。定義方法については適用条件定義を見てください。ただし、独自に定義する場合にはRubyの知識が必要になります。

適用条件は以下のように「,」でくぎって複数指定できます。

<pre>milter.applicable_conditions = ["S25R", "Remote Network"]</pre>

例:

<pre>milter.applicable_conditions = ["S25R"]</pre>

既定値:

<pre>milter.applicable_conditions = []</pre></dd>
<dt>milter.add_applicable_condition(name)</dt>
<dd>子milterを適用する条件を追加します。適用する条件についてはmilter.applicable_conditionsを見てください。

例:

<pre>milter.add_applicable_condition("S25R")</pre></dd>
<dt>milter.command</dt>
<dd>子milterを起動するコマンドを指定します。security.privilege_modeがtrueでmilter-managerコマンドがroot権限で実行されている場合、milter.connection_specへの接続が失敗した時に、子milterを自動で起動します。そのときに利用するコマンドです。

/etc/init.d/以下や/usr/local/etc/rc.d/以下にある起動スクリプトを指定することを想定しています。

コマンドは「"」で囲んで"/etc/init.d/milter-greylist"というように指定します。自動で起動しない場合はnilを指定します。

例:

<pre>milter.command = "/etc/init.d/milter-greylist"</pre>

既定値:

<pre>milter.command = nil</pre></dd>
<dt>milter.command_options</dt>
<dd>milter.commandに渡すオプションを指定します。

オプションは「"」で囲んで"start"というように指定します。複数のオプションを指定するときは"--option1 --option2"というように指定します。あるいは、全体を「[]」で囲み、それぞれのオプションを「,」で区切り、["--option1", "--option2"]というように指定することもできます。

例:

<pre>milter.command_options = "start"
milter.command_options = ["--option1", "--option2"]</pre>

既定値:

<pre>milter.command_options = nil</pre></dd>
<dt>milter.user_name</dt>
<dd>milter.commandを実行するユーザ名を指定します。

ユーザ名は「"」で囲んで"nobody"というように指定します。root権限で実行する場合はnilを指定します。

例:

<pre>milter.user_name = "nobody"</pre>

既定値:

<pre>milter.user_name = nil</pre></dd>
<dt>milter.connection_timeout</dt>
<dd>子milterに接続したときのタイムアウト時間を秒単位で指定します。

例:

<pre>milter.connection_timeout = 60</pre>

既定値:

<pre>milter.connection_timeout = 297.0</pre></dd>
<dt>milter.writing_timeout</dt>
<dd>子milterへデータを送信したときのタイムアウト時間を秒単位で指定します。

例:

<pre>milter.writing_timeout = 15</pre>

既定値:

<pre>milter.writing_timeout = 7.0</pre></dd>
<dt>milter.reading_timeout</dt>
<dd>子milterからデータを受信するときのタイムアウト時間を秒単位で指定します。

例:

<pre>milter.reading_timeout = 15</pre>

既定値:

<pre>milter.reading_timeout = 7.0</pre></dd>
<dt>milter.end_of_message_timeout</dt>
<dd>子milterからxxfi_eom()のレスポンスを受信するときのタイムアウト時間を秒単位で指定します。

例:

<pre>milter.end_of_message_timeout = 60</pre>

既定値:

<pre>milter.end_of_message_timeout = 297.0</pre></dd>
<dt>milter.name</dt>
<dd>1.8.1 から利用可能。

define_milter で設定した名前を返します。</dd></dl>

### 子milter操作

定義された子milterを操作するために便利な機能があります。ただし、これらの機能を利用するには多少Rubyの知識が必要になります。

定義されている子milter名の一覧を取得することができます。

<pre>define_milter("milter1") do |milter|
  ...
end

define_milter("milter2") do |milter|
  ...
end

defined_milters # => ["milter1", "milter2"]</pre>

これを利用することにより、すべての子milterの設定をまとめて変更するということが簡単にできるようになります。

以下はすべての子milterを無効にする例です。

<pre>defined_milters.each do |name|
  define_milter(name) do |milter|
    milter.enabled = false
  end
end</pre>

以下はすべての子milterの定義を削除にする例です。無効にした場合と違い、再び子milterを使いたい場合は一から定義しなおす必要があります。

<pre>defined_milters.each do |name|
  remove_milter(name)
end</pre>

以下はすべての子milterの適用条件にS25Rを追加する例です。

<pre>defined_milters.each do |name|
  define_milter(name) do |milter|
    milter.add_applicable_condition("S25R")
  end
end</pre>

<dl>
<dt>defined_milters</dt>
<dd>定義されている子milterの名前の一覧を返します。返される値は文字列の配列です。

例:

<pre>defined_milters # => ["milter1", "milter2"]</pre></dd>
<dt>remove_milter(name)</dt>
<dd>定義されているnameという名前のmilterを削除します。milter定義を削除せずに、単に無効にするだけでよいならmilter.enabledを使ってください。

例:

<pre># "milter1"という名前で定義されたmilterの定義を削除
remove_milter("milter1")</pre></dd></dl>

## 組み込み適用条件 {#built-in-applicable-conditions}

組み込みの適用条件を説明します。

### S25R

この適用条件を使うと、MTAっぽいSMTPクライアントには子milterを適用せず、一般PCっぽいSMTPクライアントにのみ子milterを適用します。

以下の例では一般PCっぽいSMTPクライアントにのみGreylistingを適用することで、Greylistingによる遅延の悪影響を軽減しています。これは[Rgrey](http://k2net.hakuba.jp/rgrey/)と呼ばれる手法です。（milter-greylistで"racl greylist default"と設定している場合）

使用例:

<pre>define_milter("milter-greylist") do |milter|
  milter.add_applicable_condition("S25R")
end</pre>

SMTPクライアントが一般PCっぽいかどうかは[S25R](http://www.gabacho-net.jp/anti-spam/)の一般規則を用います。

S25Rの一般規則は一般PC以外のホストにもマッチしてしまいます。そのため、明示的にホワイトリストを設定して誤検出を防ぐことができます。デフォルトではgoogle.comドメインのホストとobsmtp.comドメインのホストはS25Rの一般規則にマッチしても子milterを適用しません。

また、逆にS25Rの一般規則に規則を追加して例外的なホスト名に対応することもできます。

S25R適用条件は以下の設定でカスタマイズできます。

<dl>
<dt>s25r.add_whitelist(matcher)</dt>
<dd>1.5.2から使用可能。

<var>matcher</var>にマッチするホストをMTAと判断し、ホワイトリストに追加します。ホワイトリストに入っているホストに対しては子milterを適用しません。

<var>matcher</var>にはホスト名にマッチする正規表現、またはホスト名を文字列で指定します。

例えば、google.comドメインをホワイトリストに入れる場合は以下のようにします。

<pre>s25r.add_whitelist(/\.google\.com\z/)</pre>

mx.example.comというホストをホワイトリストに入れる場合は以下のようにします。

<pre>s25r.add_whitelist("mx.example.com")</pre>

[上級者向け] 複雑な条件を指定したい場合はブロックを指定することができます。ブロックにはホスト名が引数として渡されます。例えば、午前8:00から午後7:59までの間だけ.jpトップレベルドメインをホワイトリストに入れる場合は以下のようにします。

<pre>s25r.add_whitelist do |host|
  (8..19).include?(Time.now.hour) and /\.jp\z/ === host
end</pre></dd>
<dt>s25r.add_blacklist(matcher)</dt>
<dd>1.5.2から使用可能。

<var>matcher</var>にマッチするホストを一般PCと判断し、ブラックリストに追加します。ブラックリストに入っているホストに対して子milterを適用します。ただし、ホワイトリストとブラックリストの両方に入っている場合はホワイトリストを優先します。つまり、両方に入っている場合は子milterを適用しません。

<var>matcher</var>にはホスト名にマッチする正規表現、またはホスト名を文字列で指定します。

例えば、evil.example.comドメインをブラックリストに入れる場合は以下のようにします。

<pre>s25r.add_blacklist(/\.evil\.example\.com\z/)</pre>

black.example.comというホストをブラックリストに入れる場合は以下のようにします。

<pre>s25r.add_blacklist("black.example.com")</pre>

[上級者向け] 複雑な条件を指定したい場合はブロックを指定することができます。ブロックにはホスト名が引数として渡されます。例えば、午後8:00から午前7:59までの間だけ.jpトップレベルドメインをブラックリストに入れる場合は以下のようにします。

<pre>s25r.add_blacklist do |host|
  !(8..19).include?(Time.now.hour) and /\.jp\z/ === host
end</pre></dd>
<dt>s25r.check_only_ipv4=(boolean)</dt>
<dd>1.6.6から使用可能。

<code>true</code>を指定した場合はIPv4からの接続してきた場合のみS25Rのチェックを有効にします。<code>false</code>を指定するとIPv6からの接続の場合でもチェックします。

例:

<pre>s25r.check_only_ipv4 = false # IPv4以外のときもチェック</pre>

初期値:

<pre>IPv4の場合のみチェックする</pre></dd></dl>

### Remote Network

この適用条件を使うと、外部ネットワークからアクセスしてきたSMTPクライアントにのみ子milterを適用します。

以下の例ではローカルネットワークからのメールにはスパムチェックを行わない事で誤検出を回避しています。

使用例:

<pre>define_milter("spamass-milter") do |milter|
  milter.add_applicable_condition("Remote Network")
end</pre>

192.168.0.0/24などのプライベートIPアドレス以外を外部ネットワークとして扱います。プライベートIPアドレス以外もローカルネットワークとして扱いたい場合は以下の設定で追加することができます。

<dl>
<dt>remote_network.add_local_address(address)</dt>
<dd>1.5.0から使用可能。

指定したIPv4/IPv6アドレスまたはIPv4/IPv6ネットワークをローカルネットワークに追加します。ローカルネットワークに追加したアドレス・ネットワークには子milterを適用しません。

使用例:

<pre># 160.29.167.10からのアクセスは子milterを適用しない
remote_network.add_local_address("160.29.167.10")
# 160.29.167.0/24のネットワークからのアクセスは子milterを適用しない
remote_network.add_local_address("160.29.167.0/24")
# 2001:2f8:c2:201::fff0からのアクセスは子milterを適用しない
remote_network.add_local_address("2001:2f8:c2:201::fff0")
# 2001:2f8:c2:201::/64からのアクセスは子milterを適用しない
remote_network.add_local_address("2001:2f8:c2:201::/64")</pre></dd>
<dt>remote_network.add_remote_address(address)</dt>
<dd>1.5.0から使用可能。

指定したIPv4/IPv6アドレスまたはIPv4/IPv6ネットワークをリモートネットワークに追加します。リモートネットワークに追加したアドレス・ネットワークには子milterを適用します。

使用例:

<pre># 160.29.167.10からのアクセスは子milterを適用する
remote_network.add_remote_address("160.29.167.10")
# 160.29.167.0/24のネットワークからのアクセスは子milterを適用する
remote_network.add_remote_address("160.29.167.0/24")
# 2001:2f8:c2:201::fff0からのアクセスは子milterを適用する
remote_network.add_remote_address("2001:2f8:c2:201::fff0")
# 2001:2f8:c2:201::/64からのアクセスは子milterを適用する
remote_network.add_remote_address("2001:2f8:c2:201::/64")</pre></dd></dl>

### Authentication {#authentication}

SMTP Authで認証済みのSMTPクライアントにのみ子milterを適用します。この適用条件を利用する場合は、MTAが認証関連のマクロをmilterに渡すようにしなければいけません。Sendmailは特に設定をする必要はありません。Postfixの場合は以下の設定を追加する必要があります。

main.cf:

<pre>milter_mail_macros = {auth_author} {auth_type} {auth_authen}</pre>

以下の例は認証済みのSMTPクライアントが送ったメール（内部から送信したメール）を監査用にBccする設定です。

使用例:

<pre>define_milter("milter-bcc") do |milter|
  milter.add_applicable_condition("Authenticated")
end</pre>

### Unauthentication {#unauthentication}

SMTP Authで認証されていないSMTPクライアントにのみ子milterを適用します。Authenticationと同様に、MTAが認証関連のマクロをmilterに渡すようにしなければいけません。

以下の例は認証されていないSMTPクライアントからのメールにのみスパムチェックを行い、誤検出を回避する設定です。

使用例:

<pre>define_milter("spamass-milter") do |milter|
  milter.add_applicable_condition("Unauthenticated")
end</pre>

### Sendmail Compatible

この適用条件は少し変わっています。この適用条件を設定してもすべての子milterを適用します。では何をするのかというと、SendmailとPostfixのmilter実装の非互換を吸収し、Sendmailでしか動かないmilterをPostfixでも動くようにします。

SendmailとPostfixではmilterの実装に互換性がない部分があります。例えば、マクロ名が異なったり、マクロを渡すタイミングが異なったりします。

この適用条件を設定すると、それらの差異を吸収し、同じmilterがSendmailでもPostfixでも動作するようになります。ただし、最近のmilterはどちらでも動くように作られているのでこの適用条件が必要なくなる日は近いでしょう。これはとてもよいことです。

MTAがPostfixの場合にこの適用条件を設定しても悪影響はないので、安心してください。

以下の例はSendmail用にビルドしたmilter-greylistをPostfixでも使えるようにする設定です。

使用例:

<pre>define_milter("milter-greylist") do |milter|
  milter.add_applicable_condition("Sendmail Compatible")
end</pre>

### stress

1.5.0から使用可能。

負荷に応じて動的に処理を変更する適用条件をいくつか提供しています。負荷は同時接続数で判断します。

<dl>
<dt>stress.threshold_n_connections</dt>
<dd>1.5.0から使用可能。

負荷が高いと判断する同時接続数を返します。

Postfixを利用している場合はsmtpdの最大プロセス数を自動検出し、自動検出した最大プロセス数の3/4以上の同時接続数がある場合に負荷が高いと判断します。

Sendmailの場合は自動検出されないので、stress.threshold_n_connections=で手動で設定する必要があります。

例:

<pre># Postfixのデフォルト設定の場合（環境によって異なる）
stress.threshold_n_connections # => 75</pre></dd>
<dt>stress.threshold_n_connections=(n)</dt>
<dd>1.5.0から使用可能。

負荷が高いと判断する同時接続数を設定します。

0を指定すると常に負荷が低いと判断します。

例:

<pre># 同時接続数が75以上の場合、負荷が高いと判断
stress.threshold_n_connections = 75</pre></dd></dl>

#### No Stress {#no-stress}

1.5.0から使用可能。

負荷が小さいときのみ、子milterを適用する適用条件です。

以下の例は負荷が高いときはspamass-milterを適用しない設定です。

使用例:

<pre>define_milter("spamass-milter") do |milter|
  milter.add_applicable_condition("No Stress")
end</pre>

#### Stress Notify {#stress-notify}

1.5.0から使用可能。

負荷が高いときに"{stress}=yes"マクロを設定し、子milterに負荷が高いことを通知する適用条件です。この適用条件は通知するだけなので、設定しても常にすべての子milterは適用されます。

以下の例は負荷が高いときはmilter-greylistにマクロで通知をする設定です。

使用例:

<pre>define_milter("milter-greylist") do |milter|
  milter.add_applicable_condition("Stress Notify")
end</pre>

milter-greylist側で「負荷が高いときはGreylisting、負荷が低いときはTarpittingを使う」設定は以下のようになります。この設定を使う場合はmilter-greylist 4.3.4以降が必要です。

greylist.conf:

<pre>sm_macro "no_stress" "{stress}" unset
racl whitelist sm_macro "no_stress" tarpit 125s
racl greylist default</pre>

### Trust {#trust}

1.6.0から使用可能。

信用できるセッションには「trusted_XXX=yes」というマクロを設定します。設定されるマクロの一覧は以下の通りです。

<dl>
<dt>trusted_domain</dt>
<dd>envelope-fromのドメインが信用できる場合は「yes」になります。</dd></dl>

以下は信用するドメインに対しては、SPFの検証が成功したらそのまま受信し、失敗したらGreylistを適用する例です。

milter-manager.local.conf:

<pre>define_milter("milter-greylist") do |milter|
  milter.add_applicable_condition("Trust")
end</pre>

greylist.conf:

<pre>sm_macro "trusted_domain" "{trusted_domain}" "yes"
racl whitelist sm_macro "trusted_domain" spf pass
racl greylist sm_macro "trusted_domain" not spf pass</pre>

どの情報を使って信用するかどうかを判断するかは、以下の設定を使ってカスタマイズできます。

<dl>
<dt>trust.add_envelope_from_domain(domain)</dt>
<dd>1.6.0から使用可能。

信用するenvelope-fromのドメインを追加します。

デフォルトでは、以下のドメインを信用しています。

* gmail.com
* hotmail.com
* msn.com
* yahoo.co.jp
* softbank.ne.jp
* clear-code.com

例:

<pre>trust.add_envelope_from_domain("example.com")</pre></dd>
<dt>trust.clear</dt>
<dd>1.8.0から使用可能。

信用するenvelope-fromのドメインリストを消去します。

例:

<pre>trust.clear</pre></dd>
<dt>trust.load_envelope_from_domains(path)</dt>
<dd>1.8.0から使用可能。

<var>path</var>から信用するenvelope-fromのドメインリストを読み込みます。<var>path</var>には以下のような書式で信用するドメインを記述します。

<pre># コメント。この行は無視される。
gmail.com
# ↑の行はgmail.comを信用するという意味
/\.example\.com/
# ↑の行はexample.comのサブドメインすべてを信用するという意味

# ↑の行は空白だけの行。空行は無視する。</pre>

例:

<pre>trust.load_envelope_from_domains("/etc/milter-manager/trusted-domains.list")
# /etc/milter-manager/trusted-domains.listから信用する
# ドメインのリストを読み込みます。</pre></dd></dl>

### Restrict Accounts

条件にマッチしたenvelope-recipientを含む場合に子milterを適用します。

<dl>
<dt>restrict_accounts_by_list(*accounts, condition_name: "Restrict Accounts by List: #{accounts.inspect}", milters: defined_milters)</dt>
<dd><var>accounts</var>に子milterを適用したいアカウントの配列を与えます。最後の引数にハッシュを与えることで適用条件の名前や適用する子milterを指定することができます。内部で<var>restrict_accounts_generic</var>を呼び出しています。

例:

<pre>restrict_accounts_by_list("bob@example.com", /@example\.co\.jp\z/, condition_name: "Restrict Accounts")</pre></dd>
<dt>restrict_accounts_generic(options, &amp;restricted_account_p)</dt>
<dd><var>options</var>はハッシュで<var>condition_name</var>と<var>milters</var>というキーを指定することができます。ブロック引数は<var>context</var>と<var>recipient</var>です。</dd></dl>

## 適用条件定義 {#applicable-condition}

ここからは本格的にRubyの知識が必要になります。標準でS25Rなどの有用な適用条件は用意されています。それらで十分ではない場合は、適用条件を定義することができます。適用条件を定義することにより、子milterを適用するかを動的に判断することができます。

適用条件は以下の書式で定義します。適用条件の定義にはRubyの知識が必要になります。

<pre>define_applicable_condition("名前") do |condition|
  condition.description = ...
  condition.define_connect_stopper do |...|
    ...
  end
  ...
end</pre>

例えば、S25Rを実現する適用条件は以下のようになります。

<pre>define_applicable_condition("S25R") do |condition|
  condition.description = "Selective SMTP Rejection"

  condition.define_connect_stopper do |context, host, socket_address|
    case host
    when "unknown",
      /\A\[.+\]\z/,
      /\A[^.]*\d[^\d.]+\d.*\./,
      /\A[^.]*\d{5}/,
      /\A(?:[^.]+\.)?\d[^.]*\.[^.]+\..+\.[a-z]/i,
      /\A[^.]*\d\.[^.]*\d-\d/,
      /\A[^.]*\d\.[^.]*\d\.[^.]+\..+\./,
      /\A(?:dhcp|dialup|ppp|[achrsvx]?dsl)[^.]*\d/i
      false
    else
      true
    end
  end
end</pre>

名前解決ができなかったときはhostは"unknown"ではなく、"[IPアドレス]"になります。そのため、本当は"unknown"は必要なく、/\A\[.+\]\z/で十分ですが、念のため入れています。 :-)

define_applicable_condition do ... end内で設定できる項目は以下の通りです。

必須の項目はありません。

<dl>
<dt>condition.description</dt>
<dd>適用条件の説明を指定します。

説明は「"」で囲んで"test condition"というように指定します。

例:

<pre>condition.description = "test condition"</pre>

既定値:

<pre>condition.description = nil</pre></dd>
<dt>condition.define_connect_stopper {|context, host, socket_address| ...}</dt>
<dd>SMTPクライアントがSMTPサーバに接続してきたときのホスト名とIPアドレスを利用して子milterを適用するかどうかを判断します。このときに利用できる情報はmilterのxxfi_connectで利用可能な情報と同じです。

子milterの適用を中止する場合はtrueを返し、続ける場合はfalseを返します。

<dl>
<dt>context</dt>
<dd>その時点での様々な情報を持ったオブジェクトです。詳細は後述します。</dd>
<dt>host</dt>
<dd>接続してきたIPアドレスを名前解決して得られたホスト名（文字列）です。名前解決に失敗した場合はIPアドレスが「[]」で囲まれた文字列になります。例えば、"[1.2.3.4]"となります。</dd>
<dt>[] socket_address</dt>
<dd>接続してきたIPアドレスを表すオブジェクトです。詳細は後述します。</dd></dl>

以下はクライアントからの接続が名前解決できた場合はmilterを適用しない例です。

<pre>condition.define_connect_stopper do |context, host, socket_address|
  if /\A\[.+\]\z/ =~ host
    false
  else
    true
  end
end</pre></dd>
<dt>condition.define_helo_stopper {|context, fqdn| ...}</dt>
<dd>SMTPクライアントがHELO/EHLOのときに送ってきたFQDNを利用して子milterを適用するかどうかを判断します。このときに利用できる情報はmilterのxxfi_heloで利用可能な情報と同じです。

子milterの適用を中止する場合はtrueを返し、続ける場合はfalseを返します。

<dl>
<dt>context</dt>
<dd>その時点での様々な情報を持ったオブジェクトです。詳細は後述します。</dd>
<dt>fqdn</dt>
<dd>SMTPクライアントがHELO/EHLOのときに送ってきたFQDNです。</dd></dl>

以下はクライアントから送られてきたFQDNが"localhost.localdomain"の場合はmilterを適用しない例です。

<pre>condition.define_helo_stopper do |context, helo|
  helo == "localhost.localdomain"
end</pre></dd>
<dt>define_envelope_from_stopper {|context, from| ...}</dt>
<dd>SMTPのMAIL FROMコマンドで渡された送信元アドレスを利用して子milterを適用するかどうかを判断します。このときに利用できる情報はmilterのxxfi_envfromで利用可能な情報と同じです。

子milterの適用を中止する場合はtrueを返し、続ける場合はfalseを返します。

<dl>
<dt>context</dt>
<dd>その時点での様々な情報を持ったオブジェクトです。詳細は後述します。</dd>
<dt>from</dt>
<dd>MAIL FROMコマンドに渡された送信元です。例えば、"&lt;sender@example.com&gt;"となります。</dd></dl>

以下はexample.comから送信された場合はmilterを適用しない例です。

<pre>condition.define_envelope_from_stopper do |context, from|
  if /@example.com>\z/ =~ from
    true
  else
    false
  end
end</pre></dd>
<dt>define_envelope_recipient_stopper {|context, recipient| ...}</dt>
<dd>SMTPのRCPT TOコマンドで渡された宛先アドレスを利用して子milterを適用するかどうかを判断します。このときに利用できる情報はmilterのxxfi_envrcptで利用可能な情報と同じです。宛先が複数ある場合は複数回呼ばれます。

子milterの適用を中止する場合はtrueを返し、続ける場合はfalseを返します。

<dl>
<dt>context</dt>
<dd>その時点での様々な情報を持ったオブジェクトです。詳細は後述します。</dd>
<dt>recipient</dt>
<dd>RCPT TOコマンドに渡された宛先です。例えば、"&lt;receiver@example.com&gt;"となります。</dd></dl>

以下はml.example.com宛の場合はmilterを適用しない例です。

<pre>condition.define_envelope_recipient_stopper do |context, recipient|
  if /@ml.example.com>\z/ =~ recipient
    true
  else
    false
  end
end</pre></dd>
<dt>condition.define_data_stopper {|context| ...}</dt>
<dd>SMTPクライアントがDATAを送ってきたときに子milterを適用するかどうかを判断します。このときに利用できる情報はmilterのxxfi_dataで利用可能な情報と同じです。

子milterの適用を中止する場合はtrueを返し、続ける場合はfalseを返します。

<dl>
<dt>context</dt>
<dd>その時点での様々な情報を持ったオブジェクトです。詳細は後述します。</dd></dl>

以下はDATAまで処理が進んだらmilterを終了する例です。milterがヘッダや本文を書き換えるのはメール全体を処理した後です。DATAの時点でmilterを終了させることにより、milterがヘッダや本文を書き換えないことが保証されます。milterによっては途中の処理結果をログに出力するので、それを見てDATAまでのmilterの動作を確認することができます。

<pre>condition.define_data_stopper do |context|
  true
end</pre></dd>
<dt>define_header_stopper {|context, name, value| ...}</dt>
<dd>メールのヘッダを利用して子milterを適用するかどうかを判断します。このときに利用できる情報はmilterのxxfi_headerで利用可能な情報と同じです。各ヘッダ毎に呼ばれます。

子milterの適用を中止する場合はtrueを返し、続ける場合はfalseを返します。

<dl>
<dt>context</dt>
<dd>その時点での様々な情報を持ったオブジェクトです。詳細は後述します。</dd>
<dt>name</dt>
<dd>ヘッダ名です。例えば、"From"となります。</dd>
<dt>value</dt>
<dd>ヘッダの値です。例えば、"sender@example.com"となります。</dd></dl>

以下は"X-Spam-Flag"ヘッダの値が"YES"の場合はmilterを適用しない例です。

<pre>condition.define_header_stopper do |context, name, value|
  if ["X-Spam-Flag", "YES"] == [name, value]
    true
  else
    false
  end
end</pre></dd>
<dt>condition.define_end_of_header_stopper {|context| ...}</dt>
<dd>ヘッダをすべて処理した後に子milterを適用するかどうかを判断します。このときに利用できる情報はmilterのxxfi_eohで利用可能な情報と同じです。

子milterの適用を中止する場合はtrueを返し、続ける場合はfalseを返します。

<dl>
<dt>context</dt>
<dd>その時点での様々な情報を持ったオブジェクトです。詳細は後述します。</dd></dl>

以下はヘッダの処理が完了したらmilterを終了する例です。

<pre>condition.define_end_of_header_stopper do |context|
  true
end</pre></dd>
<dt>condition.define_body_stopper {|context, chunk| ...}</dt>
<dd>本文の一部を利用して子milterを適用するかどうかを判断します。このときに利用できる情報はmilterのxxfi_bodyで利用可能な情報と同じです。本文が大きい場合は複数回呼ばれます。

子milterの適用を中止する場合はtrueを返し、続ける場合はfalseを返します。

<dl>
<dt>context</dt>
<dd>その時点での様々な情報を持ったオブジェクトです。詳細は後述します。</dd>
<dt>chunk</dt>
<dd>本文の一部です。サイズが大きな本文は一度には処理されずに、いくつかの固まりに分割されて処理されます。最大で65535バイトのデータになります。</dd></dl>

以下は本文が署名されていたらmilterを終了する例です。

<pre>condition.define_body_stopper do |context, chunk|
  if /^-----BEGIN PGP SIGNATURE-----$/ =~ chunk
    true
  else
    false
  end
end</pre></dd>
<dt>condition.define_end_of_message_stopper {|context| ...}</dt>
<dd>本文をすべて処理した後に子milterを適用するかどうかを判断します。このときに利用できる情報はmilterのxxfi_eomで利用可能な情報と同じです。

子milterの適用を中止する場合はtrueを返し、続ける場合はfalseを返します。

<dl>
<dt>context</dt>
<dd>その時点での様々な情報を持ったオブジェクトです。詳細は後述します。</dd></dl>

以下は本文の処理が完了したらmilterを終了する例です。

<pre>condition.define_end_of_message_stopper do |context|
  true
end</pre></dd></dl>

### context

子milterを適用するかどうかを判断する時点での様々な情報を持ったオブジェクトです。（クラスはMilter::Manager::ChildContextです。）

以下の情報を持っています。

<dl>
<dt>context.name</dt>
<dd>子milterの名前です。define_milterで使った名前になります。

例:

<pre>context.name # -> "clamav-milter"</pre></dd>
<dt>context[name]</dt>
<dd>子milterが利用可能なマクロの値を返します。libmilterのAPIでは1文字より長いマクロ名の場合は「{}」で囲まなければいけませんが、context[]では囲んでも囲まなくてもどちらでも構いません。

例:

<pre>context["j"] # -> "mail.example.com"
context["rcpt_address"] # -> "receiver@example.com"
context["{rcpt_address}"] # -> "receiver@example.com"</pre></dd>
<dt>context.reject?</dt>
<dd>子milterがrejectを返したときにtrueを返します。子milterはmilter.evaluation_modeを有効にしてください。

引数として渡ってくるcontextは処理中のため、context.reject?がtrueを返すことはありません。context.children[]で取得した別の子milterの結果を利用するときに有用です。

例:

<pre>context.reject? # -> false
context.children["milter-greylist"].reject? # -> true or false</pre></dd>
<dt>context.temporary_failure?</dt>
<dd>子milterがtemporary failureを返したときにtrueを返します。子milterはmilter.evaluation_modeを有効にしてください。

引数として渡ってくるcontextは処理中のため、context.temporary_failure?がtrueを返すことはありません。context.children[]で取得した別の子milterの結果を利用するときに有用です。

例:

<pre>context.temporary_failure? # -> false
context.children["milter-greylist"].temporary_failure? # -> true or false</pre></dd>
<dt>context.accept?</dt>
<dd>子milterがacceptを返したときにtrueを返します。

引数として渡ってくるcontextは処理中のため、context.accept?がtrueを返すことはありません。context.children[]で取得した別の子milterの結果を利用するときに有用です。

例:

<pre>context.accept? # -> false
context.children["milter-greylist"].accept? # -> true or false</pre></dd>
<dt>context.discard?</dt>
<dd>子milterがdiscardを返したときにtrueを返します。子milterはmilter.evaluation_modeを有効にしてください。

引数として渡ってくるcontextは処理中のため、context.discard?がtrueを返すことはありません。context.children[]で取得した別の子milterの結果を利用するときに有用です。

例:

<pre>context.discard? # -> false
context.children["milter-greylist"].discard? # -> true or false</pre></dd>
<dt>context.quitted?</dt>
<dd>子milterの処理が終了している場合にtrueを返します。

引数として渡ってくるcontextは処理中のため、context.quitted?は常にfalseです。context.children[]で取得した別の子milterの結果を利用するときに有用です。

例:

<pre>context.quitted? # -> false
context.children["milter-greylist"].quitted? # -> true or false</pre></dd>
<dt>context.children[name]</dt>
<dd>別の子milterのcontextを取得します。

別の子milterを参照するときに利用する名前はdefine_milterで使った名前（context.nameで取得できる名前）になります。

存在しない名前で参照しようとした場合はnilが返ります。

例:

<pre>context.children["milter-greylist"] # -> milter-greylistのcontext
context.children["nonexistent"]     # -> nil
context.children[context.name]      # -> 自分のcontext</pre></dd>
<dt>context.postfix?</dt>
<dd>MTAがPostfixの場合に真を返します。Postfixかどうかは「v」マクロの値に「Postfix」という文字列が含まれるかどうかで判断します。

Postfixの場合はtrue、そうでない場合はfalseが返ります。

例:

<pre>context["v"]     # -> "Postfix 2.5.5"
context.postfix? # -> true

context["v"]     # -> "2.5.5"
context.postfix? # -> false

context["v"]     # -> nil
context.postfix? # -> false</pre></dd>
<dt>context.authenticated?</dt>
<dd>送信元が認証されている場合に真を返します。認証されているかどうかは「auto_type」マクロか「auth_authen」マクロの値があるかで判断します。これらのマクロはMAIL FROM以降でのみ使えるので、それ以前の場合は常に偽を返します。Postfixの場合はmain.cfに以下を追加することを忘れないで下さい。

<pre>milter_mail_macros = {auth_author} {auth_type} {auth_authen}</pre>

認証されている場合はtrue、そうでない場合はfalseが返ります。

例:

<pre>context["auth_type"]   # -> nil
context["auth_authen"] # -> nil
context.authenticated? # -> false

context["auth_type"]   # -> "CRAM-MD5"
context["auth_authen"] # -> nil
context.authenticated? # -> true

context["auth_type"]   # -> nil
context["auth_authen"] # -> "sender"
context.authenticated? # -> true</pre></dd>
<dt>context.mail_transaction_shelf</dt>
<dd>メールトランザクションFIXMEの間、子milter同士で共有するデータを保存します。例えば、envelope-recipient にあるユーザが含まれる場合のみ、子 milter でメール本文を処理する、というような使い方ができます。

2.0.5 から使えます[実験的]

例:

<pre>define_applicable_condition("") do |condition|
  condition.define_envelope_recipient_stopper do |context, recipient|
    if /\Asomeone@example.com\z/ =~ recipient
      context.mail_transaction_shelf["stop-on-data"] = true
    end
    false
  end
  condition.define_data_stopper do |context|
    context.mail_transaction_shelf["stop-on-data"]
  end
end</pre></dd></dl>

### socket_address {#socket-address}

ソケットのアドレスを表現しているオブジェクトです。IPv4ソケット、IPv6ソケット、UNIXドメインソケットそれぞれで別々のオブジェクトになります。

#### Milter::SocketAddress::IPv4

IPv4ソケットのアドレスを表現するオブジェクトです。以下のメソッドを持ちます。

<dl>
<dt>address</dt>
<dd>ドット表記のIPv4アドレスを返します。

例:

<pre>socket_address.address # -> "192.168.1.1"</pre></dd>
<dt>port</dt>
<dd>ポート番号を返します。

例:

<pre>socket_address.port # -> 12345</pre></dd>
<dt>to_s</dt>
<dd>connection_specと同じ書式で表現したIPv4アドレスを返します。

例:

<pre>socket_address.to_s # -> "inet:12345@[192.168.1.1]"</pre></dd>
<dt>local?</dt>
<dd>プライベートなネットワークのアドレスの場合はtrueを返します。

例:

<pre>socket_address.to_s   # -> "inet:12345@[127.0.0.1]"
socket_address.local? # -> true

socket_address.to_s   # -> "inet:12345@[192.168.1.1]"
socket_address.local? # -> true

socket_address.to_s   # -> "inet:12345@[160.XXX.XXX.XXX]"
socket_address.local? # -> false</pre></dd>
<dt>to_ip_address</dt>
<dd>対応するIPAddrオブジェクトを返します。

例:

<pre>socket_address.to_s          # -> "inet:12345@[127.0.0.1]"
socket_address.to_ip_address # -> #<IPAddr: IPv4:127.0.0.1/255.255.255.255></pre></dd></dl>

#### Milter::SocketAddress::IPv6

IPv6ソケットのアドレスを表現するオブジェクトです。以下のメソッドを持ちます。

<dl>
<dt>address</dt>
<dd>コロン表記のIPv6アドレスを返します。

例:

<pre>socket_address.address # -> "::1"</pre></dd>
<dt>port</dt>
<dd>ポート番号を返します。

例:

<pre>socket_address.port # -> 12345</pre></dd>
<dt>to_s</dt>
<dd>connection_specと同じ書式で表現したIPv6アドレスを返します。

例:

<pre>socket_address.to_s # -> "inet6:12345@[::1]"</pre></dd>
<dt>local?</dt>
<dd>プライベートなネットワークのアドレスの場合はtrueを返します。

例:

<pre>socket_address.to_s   # -> "inet6:12345@[::1]"
socket_address.local? # -> true

socket_address.to_s   # -> "inet6:12345@[fe80::XXXX]"
socket_address.local? # -> true

socket_address.to_s   # -> "inet6:12345@[2001::XXXX]"
socket_address.local? # -> false</pre></dd>
<dt>to_ip_address</dt>
<dd>対応するIPAddrオブジェクトを返します。

例:

<pre>socket_address.to_s          # -> "inet6:12345@[::1]"
socket_address.to_ip_address # -> #<IPAddr: IPv6:0000:0000:0000:0000:0000:0000:0000:0001/ffff:ffff:ffff:ffff:ffff:ffff:ffff:ffff></pre></dd></dl>

#### Milter::SocketAddress::Unix

UNIXドメインソケットのアドレスを表現するオブジェクトです。以下のメソッドを持ちます。

<dl>
<dt>path</dt>
<dd>ソケットのパスを返します。

例:

<pre>socket_address.path # -> "/tmp/local.sock"</pre></dd>
<dt>to_s</dt>
<dd>connection_specと同じ書式で表現したUNIXドメインソケットアドレスを返します。

例:

<pre>socket_address.to_s # -> "unix:/tmp/local.sock"</pre></dd>
<dt>local?</dt>
<dd>常にtrueを返します。

例:

<pre>socket_address.local? # -> true</pre></dd>
<dt>to_ip_address</dt>
<dd>常にnilを返します。

例:

<pre>socket_address.to_s          # -> "unix:/tmp/local.sock"
socket_address.to_ip_address # -> nil</pre></dd></dl>

## データベース関連 {#database}

1.6.6から使用可能。

データベース操作機能を利用する場合はRubyの知識が必要になります。データベース操作ライブラリとして[ActiveRecord](http://api.rubyonrails.org/files/activerecord/README_rdoc.html)を採用しています。そのため、MySQLやSQLite3など多くのRDBを操作することができます。

データベース接続機能を利用する場合はActiveRecordを別途インストールする必要があります。ActiveRecordのインストールにはRuby用のパッケージ管理システム[RubyGems](https://rubygems.org/)を用います。RubyGemsとActiveRecordのインストール方法はインストールページにあるインストールドキュメントのうち、「（任意）」の方のインストールドキュメントを参照してください。それぞれの環境でのRubyGemsとActiveRecordのインストール方法とを説明しています。

ActiveRecordをインストールしたらデータベース操作機能を利用することができます。

MySQLのusersテーブルの値を操作する場合の例を示します。

MySQLの接続情報は以下の通りとします。

<dl>
<dt>データベース名</dt>
<dd>mail-system</dd>
<dt>データベースサーバのIPアドレス</dt>
<dd>192.168.0.1</dd>
<dt>ユーザ名</dt>
<dd>milter-manager</dd>
<dt>パスワード</dt>
<dd>secret</dd></dl>

まず、この接続情報をmilter-manager.local.confで指定します。ここでは、milter-manager.local.confは/etc/milter-manager/milter-manager.local.confにあるとします。

/etc/milter-manager/milter-manager.local.conf:

<pre>database.type = "mysql2"
database.host = "192.168.0.1"
database.name = "mail-system"
database.user = "milter-manager"
database.password = "secret"</pre>

次に、usersテーブルに接続するためのActiveRecrodオブジェクトを定義します。定義ファイルはmilter-manager.local.confが置いてあるディレクトリと同じパスにあるmodels/ディレクトリ以下に置きます。今回はusersテーブル用の定義ファイルなのでmodels/user.rbを作成します。

/etc/milter-manager/models/user.rb:

<pre>class User < ActiveRecord::Base
end</pre>

これで準備は整ったので、再び、milter-manager.local.confへ戻ります。以下のように書いてデータベースへ接続し、データを操作します。

/etc/milter-manager/milter-manager.local.conf:

<pre>database.setup
database.load_models("models/*.rb")
User.all.each do |user|
  p user.name # => "alice", "bob", ...
end</pre>

まとめると以下のようになります。

/etc/milter-manager/milter-manager.local.conf:

<pre># 接続情報設定
database.type = "mysql2"
database.host = "192.168.0.1"
database.name = "mail-system"
database.user = "milter-manager"
database.password = "secret"

# 接続
database.setup

# 定義を読み込み
database.load_models("models/*.rb")
# データを操作
User.all.each do |user|
  p user.name # => "alice", "bob", ...
end</pre>

/etc/milter-manager/models/user.rb:

<pre>class User < ActiveRecord::Base
end</pre>

以下は設定項目です。

<dl>
<dt>database.type</dt>
<dd>データベースの種類を指定します。

指定可能な種類は以下の通りです。

<dl>
<dt>"mysql2"</dt>
<dd>MySQLを利用します。以下のようにmysql2 gemをインストールする必要があります。

<pre>% sudo gem install mysql2</pre></dd>
<dt>"sqlite3"</dt>
<dd>SQLite3を利用します。以下のようにsqlite3 gemをインストールする必要があります。

<pre>% sudo gem install sqlite3</pre></dd>
<dt>"pg"</dt>
<dd>PostgreSQLを利用します。以下のようにpg gemをインストールする必要があります。

<pre>% sudo gem install pg</pre></dd></dl>

例:

<pre>database.type = "mysql2" # MySQLを利用</pre></dd>
<dt>database.name</dt>
<dd>接続するデータベース名を指定します。

SQLite3ではデータベースのパスまたは<code>":memory:"</code>を指定します。

例:

<pre>database.name = "configurations" # configurationsデータベースへ接続</pre></dd>
<dt>database.host</dt>
<dd>接続するデータベースサーバのホスト名を指定します。

MySQLなどでは規定値として"localhost"を利用します。

SQLite3では無視されます。

例:

<pre>database.host = "192.168.0.1" # 192.168.0.1で動いているサーバへ接続</pre></dd>
<dt>database.port</dt>
<dd>接続するデータベースサーバのポート番号を指定します。

それぞれのRDBごとに規定値が設定されているため、多くの場合、明示的に指定する必要はありません。

SQLite3では無視されます。

例:

<pre>database.port = 3306 # 3306番ポートで動いているサーバへ接続</pre></dd>
<dt>database.path</dt>
<dd>接続するデータベースサーバのUNIXドメインソケットのパスを指定します。

SQLite3の場合はデータベースのパスになります。ただし、.#database.nameの設定の方が優先されるため、.#database.pathではなく、.#database.nameを使うことを推奨します。

例:

<pre>database.path = "/var/run/mysqld/mysqld.sock" # UNIXドメインソケットでMySQLへ接続</pre></dd>
<dt>database.user</dt>
<dd>データベース接続ユーザを指定します。

SQLite3では無視されます。

例:

<pre>database.user = "milter-manager" # milter-managerユーザでサーバへ接続</pre></dd>
<dt>database.password</dt>
<dd>データベース接続時に使うパスワードを指定します。

SQLite3では無視されます。

例:

<pre>database.password = "secret"</pre></dd>
<dt>database.extra_options</dt>
<dd>1.6.9から使用可能。

追加のオプションを指定します。例えば、ActiveRecordのMySQL2アダプタにある<code>:reconnect</code>オプションを指定する場合は以下のようになります。

<pre>database.type = "mysql2"
database.extra_options[:reconnect] = true</pre>

指定できるオプションはデータベース毎に異なります。

例:

<pre>database.extra_options[:reconnect] = true</pre></dd>
<dt>database.setup</dt>
<dd>データベースへ接続します。

この時点ではじめてデータベースへ接続します。この後からデータベースを操作できるようになります。

例:

<pre>database.setup</pre></dd>
<dt>database.load_models(path)</dt>
<dd>ActiveRecord用のクラス定義が書かれたRubyスクリプトを読み込みます。<var>path</var>にはglobが使えるため、"models/*.rb"として複数のファイルを一度に読み込むことができます。<var>path</var>が相対パスだった場合は、milter-manager.confがあるディレクトリからの相対パスになります。

例:

<pre># /etc/milter-manager/models/user.rb
# /etc/milter-manager/models/group.rb
# などを読み込む。
# （/etc/milter-manager/milter-manager.confを使っている場合）
database.load_models("models/*.rb")</pre></dd></dl>


