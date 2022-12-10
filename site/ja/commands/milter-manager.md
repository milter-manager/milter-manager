---
title: milter-manager / milter manager / milter managerのマニュアル
---

# milter-manager / milter manager / milter managerのマニュアル

## 名前

milter-manager - milterを使った効果的なスパム対策・ウィルス対策

## 書式

<code>milter-manager</code> [<em>オプション ...</em>]

## 説明

milter-managerはmilterを使った効果的なスパム対策・ウィルス対策（迷惑メール対策）を実現するmilterです。

milter-managerはmilterを効果的でかつ柔軟にmilterを使うためのプラットフォームを提供します。milter-managerはRubyインタプリタを組み込んでいて、動的にmilterを適用する条件を決定することができます。この組み込まれたRubyインタプリタを使うことで、効果的でかつ柔軟にmilterを使うプラットフォームを提供することができます。

milter-managerには設定ファイルがあります。現在の設定は--show-configオプションで確認できます。

<pre>% milter-manager --show-config</pre>

milter-manager設定ファイルで指定された設定を上書きするオプションも提供しています。

## オプション

<dl>
<dt>--help</dt>
<dd>利用できるオプションを表示して終了します。</dd>
<dt>--connection-spec=SPEC</dt>
<dd>接続を待ち受けるソケットを指定します。SPECの書式は次のうちのどれかです。

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
* inet6:10025@[::1]

設定ファイル中の「manager.connection_spec」の値を上書きします。</dd>
<dt>--config-dir=DIRECTORY</dt>
<dd>設定ファイルのあるディレクトリを指定します。milter-managerは、まず、DIRECTORY/milter-manager.confの読み込みを試みます。もし、見つからなかった場合はシステム標準の場所にあるmilter-manager.confを読み込みます。</dd>
<dt>--pid-file=FILE</dt>
<dd>milter-managerのプロセスidをFILEに保存します。

設定ファイル中の「manager.pid_file」の値を上書きします。</dd>
<dt>--user-name=NAME</dt>
<dd>milter-managerをNAMEユーザの権限で実行します。milter-managerはroot権限で起動しなければいけません。

設定ファイル中の「security.effective_user」の値を上書きします。</dd>
<dt>--group-name=NAME</dt>
<dd>milter-managerをNAMEグループの権限で実行します。milter-managerはroot権限で起動しなければいけません。

設定ファイル中の「security.effective_group」の値を上書きします。</dd>
<dt>--socket-group-name=NAME</dt>
<dd>milter-managerが接続を受け付けるUNIXドメインソケットのグループをNAMEグループに変更します。グループは実効ユーザの補助グループでなければいけません。

設定ファイル中の「manager.unix_socket_group」の値を上書きします。</dd>
<dt>--daemon</dt>
<dd>端末を切り離し、バックグラウンドでデーモンプロセスとして動作します。運用時はデーモンプロセスで動作させることをお勧めします。

設定ファイル中の「manager.daemon」の値を上書きします。</dd>
<dt>--no-daemon</dt>
<dd>このオプションより前に指定した--daemonオプションを無効にします。</dd>
<dt>--show-config</dt>
<dd>設定内容を表示して終了します。設定内容はそのまま設定ファイルに使える書式で表示されます。登録されているmilterを確認する場合や、milter-managerの問題を報告する場合などで有用です。</dd>
<dt>--log-level=LEVEL</dt>
<dd>ログに出力する項目を指定します。複数の項目を指定する場合は「|」で区切って「error|warning|message」というようにします。

LEVELに指定できる項目はログ一覧 - 出力レベルを参照してください。</dd>
<dt>--log-path=PATH</dt>
<dd>ログの出力先のパスを指定します。指定しない場合は標準出力に出力します。「-」を指定しても標準出力に出力します。</dd>
<dt>--event-loop-backend=BACKEND</dt>
<dd>イベントループのバックエンドを指定します。<var>BACKEND</var>に指定できる値は、<kbd>glib</kbd>か<kbd>libev</kbd>のいずれかです。glibをバックエントとして使う場合、以下の諸注意に目を通してください。

注意: milter-managerは1プロセスあたりの処理性能を高める都合上、イベント駆動ベースのモデルで実装されています。glibを用いて実現する場合、コールバックにより表現されます。この時、glibのコールバックの登録数の上限により通信回数が制限されることに注意してください。この制限事項はglibバックエンド対してのみあります。</dd>
<dt>--verbose</dt>
<dd>実行時のログをより詳細に出力します。ログはsyslogのmailファシリティで出力します。デーモンプロセスになっていない場合は標準出力にもログが出力されます。

「--log-level=all」を指定した場合と同じ効果があります。</dd>
<dt>--version</dt>
<dd>バージョンを表示して終了します。</dd></dl>

## 終了ステータス

MTAからの接続を受け付ける状態になった場合は0になり、そうでない場合は0以外になります。milter-managerはソケットの書式が間違っている場合か、ソケットまわりの問題が起こると接続を受け付ける状態になれません。ソケットまわりの問題とは、例えば、ポートがすでに使用されているとか、UNIXドメインソケットを作成するパーミッションがない、などです。

## ファイル

<dl>
<dt>/usr/local/etc/milter-manager/milter-manager.conf</dt>
<dd>デフォルトの設定ファイルです。</dd></dl>

## シグナル

milter-managerは以下のシグナルを処理します。

<dl>
<dt>SIGHUP</dt>
<dd>設定ファイルを読み込み直します。</dd>
<dt>SIGUSR1</dt>
<dd>ログファイルを開き直します。</dd></dl>

## 例

以下はmilter-managerの挙動をデバッグするときの例です。この場合、milter-managerはフォアグラウンドで動作し、標準出力にログを吐きます。

<pre>% milter-manager --no-daemon --verbose</pre>

## 関連項目

milter-test-server.rd(1),milter-test-client.rd(1),milter-performance-check.rd(1),milter-manager-log-analyzer.rd(1)


