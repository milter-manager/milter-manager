---
title: milter-test-client / milter manager / milter managerのマニュアル
---

# milter-test-client / milter manager / milter managerのマニュアル

## 名前

milter-test-client - milter側のmilterプロトコルを実装したプログラム

## 書式

<code>milter-test-client</code> [<em>オプション ...</em>]

## 説明

milter-test-clientはMTA側から渡ってきた情報を表示するだけのmilterです。マクロも含めてMTA側から渡っている情報を表示するので、MTA側のmilterの設定を確認するために利用できます。

Postfixのアーカイブの中にも同様のツールがあります。src/milter/内にあるtest-milter.cがそれで、Postfixのmilterの実装のテストにも使っているようです。ただし、test-milterはマクロまでは表示してくれません。もし、マクロを利用しているmilterが期待通りに動かない場合の問題調査を行うのであれば、milter-test-clientが役立つでしょう。

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

* unix:/tmp/milter-test-client.sock
* inet:10025
* inet:10025@localhost
* inet:10025@[127.0.0.1]
* inet6:10025
* inet6:10025@localhost
* inet6:10025@[::1]</dd>
<dt>--log-level=LEVEL</dt>
<dd>ログに出力する項目を指定します。複数の項目を指定する場合は「|」で区切って「error|warning|message」というようにします。

LEVELに指定できる項目はログ一覧 - 出力レベルを参照してください。</dd>
<dt>--log-path=PATH</dt>
<dd>ログの出力先のパスを指定します。指定しない場合は標準出力に出力します。「-」を指定しても標準出力に出力します。</dd>
<dt>--verbose</dt>
<dd>実行時のログをより詳細に出力します。

「--log-level=all」を指定した場合と同じ効果があります。</dd>
<dt>--syslog</dt>
<dd>Syslogにもログを出力します。</dd>
<dt>--no-report-request</dt>
<dd>MTAから受け取った情報を表示しません。</dd>
<dt>--report-memory-profile</dt>
<dd>milterセッションが終了する毎にメモリ使用量を表示します。

MILTER_MEMORY_PROFILE環境変数をyesに設定するとより詳細な情報を表示します。

例:

<pre>% MILTER_MEMORY_PROFILE=yes milter-test-client -s inet:10025</pre></dd>
<dt>--daemon</dt>
<dd>デーモンプロセスとして起動します。</dd>
<dt>--user=USER</dt>
<dd>実効ユーザをUSERとします。root権限が必要です。</dd>
<dt>--group=GROUP</dt>
<dd>実効グループをGROUPとします。root権限が必要です。</dd>
<dt>--unix-socket-group=GROUP</dt>
<dd>SPECで「unix:パス」を指定したときにUNIXドメインソケットのグループを変更します。</dd>
<dt>--n-workers=N_WORKERS</dt>
<dd>メールを処理するプロセスを<var>N_WORKERS</var>個起動します。指定できる値は0以上、1000以下です。0のときにワーカープロセスを使用しません。

<em>注意: この項目は実験的な機能扱いです。</em></dd>
<dt>--event-loop-backend=BACKEND</dt>
<dd>イベントループのバックエンドを指定します。<var>BACKEND</var>に指定できる値は、<kbd>glib</kbd>か<kbd>libev</kbd>のいずれかです。glibをバックエントとして使う場合、以下の諸注意に目を通してください。

<em>注意: milter-managerは1プロセスあたりの処理性能を高める都合上、イベント駆動ベースのモデルで実装されています。glibを用いて実現する場合、コールバックにより表現されます。この時、glibのコールバックの登録数の上限により通信回数が制限されることに注意してください。この制限事項はglibバックエンド対してのみあります。</em></dd>
<dt>--packet-buffer-size=SIZE</dt>
<dd>end-of-message時に送信パケットをバッファリングするためのバッファサイズを指定します。パケットの量が<var>SIZE</var>バイト以上になるとまとめてパケットを送信します。0を指定するとバッファリングしません。

既定値は0KBで、バッファリングしません。</dd>
<dt>--version</dt>
<dd>バージョンを表示して終了します。</dd></dl>

## 終了ステータス

MTAからの接続を受け付ける状態になった場合は0になり、そうでない場合は0以外になります。milter-test-clientはソケットの書式が間違っている場合か、ソケットまわりの問題が起こると接続を受け付ける状態になれません。ソケットまわりの問題とは、例えば、ポートがすでに使用されているとか、UNIXドメインソケットを作成するパーミッションがない、などです。

## 例

以下の例では、10025番ポートでMTAからの接続を待つmilterを起動します。

<pre>% milter-test-client -s inet:10025</pre>

## 関連項目

milter-test-server.rd.ja(1),milter-performance-check.rd.ja(1)


