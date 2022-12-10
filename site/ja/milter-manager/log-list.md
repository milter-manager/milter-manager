---
title: ログ一覧
---

# ログ一覧 --- milter managerが出力するログの一覧

## このドキュメントについて

milter managerが出力するログについて説明します。

## モジュール

milter managerは以下の4つのモジュールに分かれています。

* core
* client
* server
* manager

それぞれ以下のような役割になっています。

<dl>
<dt>core</dt>
<dd>client, server, namagerのモジュールで共通に利用する機能を提供するモジュールです。入出力や通信データのエンコード・デコード処理などを含んでいます。</dd>
<dt>client</dt>
<dd>milterを実装するために必要な機能を提供するモジュールです。coreを利用しています。</dd>
<dt>server</dt>
<dd>MTA側のmilter通信部分を実装するために必要な機能を提供するモジュールです。coreを利用しています。</dd>
<dt>manager</dt>
<dd>milter managerを実装するために必要な機能を提供するモジュールです。core, client, serverを利用しています。</dd></dl>

libmilter-clientを利用して実装したmilterはclientモジュールを利用しているため、coreとclientのログが出力されます。miltermanagerはmanagerモジュールを利用しているため、core, client,server, managerすべてのモジュールのログが出力されます。

## 出力レベル {#level}

以下の出力レベルがあり、必要な情報のみログ出力することができます。複数の情報を出力したい場合は複数の出力レベルを指定します。

* default: critical, error, warningを出力
* none: 出力しない
* critical: 致命的な問題のみ出力
* error: エラーのみ出力
* warning: 警告のみ出力
* info: 付加情報のみ出力
* debug: デバッグ情報のみ出力
* statistics: 統計情報のみ出力
* profile: プロファイル情報のみ出力
* all: すべての情報を出力

## フォーマット

ログは以下のようなフォーマットになっています。

<pre>[#{セッションID}] [#{タグ名1}][#{タグ名2}][...] #{メッセージ}</pre>

このうち、「セッションID」と「メッセージ」は省略されている場合があります。

<dl>
<dt>セッションID</dt>
<dd>セッションIDは数値です。セッション毎に重複しない値が使われ、モジュールが異なっていてもセッションが同じであれば同じセッションIDを用います。</dd>
<dt>タグ名</dt>
<dd>タグ名にはアルファベット・数字・ハイフン（-）・アンダーバー（_）のみ利用します。利用しているタグ名には特に規則はありません。</dd>
<dt>メッセージ</dt>
<dd>メッセージは任意の文字が入ります。</dd></dl>

以下は「セッションID」が省略されている例です。

<pre>[agent][error][decode] Decode error</pre>

このログには下のタグがついています。

* agent
* error
* decode

また、このログのメッセージは以下の通りです。

<pre>Decode error</pre>

「セッションID」がつくと以下のようになります。

<pre>[29] [agent][error][decode] Decode error</pre>

## ログ一覧

モジュール毎にログの「タグ名」とログが出力される条件を説明します。

以下のフォーマットで記述します。

<dl>
<dt>#{出力レベル}: [#{タグ名1}][#{タグ名2}][...]</dt>
<dd>#{ログが出力される条件}</dd></dl>

例えば、以下のようになります。

<dl>
<dt>error: [reader][error][read]</dt>
<dd>読み込み時にエラーが発生。</dd></dl>

これは、「読み込み時にエラーが発生」したときに「[reader][error][read]」というタグの付いたログが「出力レベルerror」で出力されるということを表します。

### core

coreモジュールのログの一覧です。

coreモジュールには以下のオブジェクト（ひとまとまりの処理を実行する単位）があります。タグにオブジェクト名が含まれます。

* reader: データ読み込みオブジェクト。
* writer: データ書き込みオブジェクト。
* agent: データのやりとりをするオブジェクト。データの読み    書きにreaderとwriterを使う。

<dl>
<dt>error: [reader][error][read]</dt>
<dd>読み込み時にエラーが発生。</dd>
<dt>error: [reader][callback][error]</dt>
<dd>入力エラーが発生。</dd>
<dt>error: [reader][watch][read][fail]</dt>
<dd>読み込み可能検出監視の登録に失敗。</dd>
<dt>error: [reader][watch][error][fail]</dt>
<dd>入力エラー検出監視の登録に失敗。</dd>
<dt>error: [reader][error][shutdown]</dt>
<dd>入力の終了処理に失敗。</dd>
<dt>error: [writer][flush-callback][error]</dt>
<dd>出力データのフラッシュ時にエラーが発生。</dd>
<dt>error: [writer][write-callback][error]</dt>
<dd>書き込み時にエラーが発生。</dd>
<dt>error: [writer][write][error]</dt>
<dd>書き込みリクエスト時にエラーが発生。</dd>
<dt>error: [writer][flush][error]</dt>
<dd>出力データのフラッシュリクエスト時にエラーが発生。</dd>
<dt>error: [writer][error-callback][error]</dt>
<dd>出力エラーが発生。</dd>
<dt>error: [writer][watch][fail]</dt>
<dd>出力エラー検出監視の登録に失敗。</dd>
<dt>error: [writer][shutdown][flush-buffer][write][error]</dt>
<dd>出力終了処理直前の未送信データ出力時にエラーが発生。</dd>
<dt>error: [writer][shutdown][flush-buffer][flush][error]</dt>
<dd>出力終了処理直前の未送信データのフラッシュ時にエラーが発生。</dd>
<dt>error: [writer][error][shutdown]</dt>
<dd>出力の終了処理に失敗。</dd>
<dt>error: [agent][error][decode]</dt>
<dd>入力データの解析時にエラーが発生。</dd>
<dt>error: [agent][error][reader]</dt>
<dd>入力データの読み込み時にエラーが発生。</dd>
<dt>error: [agent][error][writer]</dt>
<dd>出力データの書き込み時にエラーが発生。</dd>
<dt>error: [agent][error][set-writer][auto-flush]</dt>
<dd>出力先切替時の自動フラッシュ時にエラーが発生。</dd></dl>

### client

clientモジュールのログの一覧です。

セッションに関係ない部分では「セッションID」は付きません。

<dl>
<dt>error: [client][connection-spec][default][error]</dt>
<dd>デフォルトの接続情報の設定に失敗。</dd>
<dt>error: [client][error][write]</dt>
<dd>出力データの書き込み時にエラーが発生。</dd>
<dt>error: [client][error][buffered-packets][write]</dt>
<dd>溜めていた出力データの書き込み時にエラーが発生。</dd>
<dt>error: [client][error][reply-on-end-of-message][flush]</dt>
<dd>end-of-messageイベント応答時の自動フラッシュでエラーが発生。</dd>
<dt>error: [client][error][unix]</dt>
<dd>UNIXドメインソケットの初期化中にエラーが発生。</dd>
<dt>error: [client][unix][error]</dt>
<dd>UNIXドメインソケットの終了処理中にエラーが発生。</dd>
<dt>error: [client][single-thread][start][error]</dt>
<dd>シングルスレッドモードでのメインループ開始時にエラーが発生。</dd>
<dt>error: [client][multi-thread][start][error]</dt>
<dd>マルチスレッドモードでのメインループ開始時にエラーが発生。</dd>
<dt>error: [client][main][error]</dt>
<dd>処理開始時にエラーが発生。</dd>
<dt>warning: [client][accept][suspend]</dt>
<dd>同時接続数が多すぎて（RLIMIT_NOFILEが足りない）新しく接続を受け付けられず、一時的に接続受付を中断している。</dd>
<dt>warning: [client][accept][resume]</dt>
<dd>一時中断していた接続受付を再開。</dd>
<dt>warning: [client][error][accept]</dt>
<dd>接続受付に失敗。</dd>
<dt>error: [client][single-thread][accept][start][error]</dt>
<dd>シングルスレッドモードでの接続受付の開始に失敗。</dd>
<dt>error: [client][multi-thread][accept][error]</dt>
<dd>マルチスレッドモードでの接続受付の開始に失敗。</dd>
<dt>error: [client][multi-thread][error]</dt>
<dd>スレッドプールにスレッド追加時にエラーが発生。</dd>
<dt>error: [client][watch][error]</dt>
<dd>listen中のソケットにエラーが発生。</dd>
<dt>error: [client][prepare][error]</dt>
<dd>初期化時にエラーが発生。</dd>
<dt>error: [client][prepare][listen][error]</dt>
<dd>listen(2)に失敗。</dd>
<dt>error: [client][pid-file][error][remove]</dt>
<dd>PIDファイルの削除時にエラーが発生。</dd>
<dt>error: [client][pid-file][save][error]</dt>
<dd>PIDファイルの保存時にエラーが発生。</dd>
<dt>error: [client][run][success][cleanup][error]</dt>
<dd>正常終了時の後処理でエラーが発生。</dd>
<dt>error: [client][run][fail][cleanup][error]</dt>
<dd>異常終了時の後処理でエラーが発生。</dd>
<dt>error: [client][master][run][error]</dt>
<dd>マスタープロセスの処理開始時にエラーが発生。</dd>
<dt>error: [client][worker][run][error]</dt>
<dd>ワーカープロセスの処理開始時にエラーが発生。</dd>
<dt>error: [client][worker][run][listen][error]</dt>
<dd>マルチワーカーモード時にワーカープロセスのlisten(2)でエラーが発生。</dd>
<dt>error: [client][workers][run][listen][error]</dt>
<dd>マルチワーカーモード時にマスタープロセスのlisten(2)でエラーが発生。</dd>
<dt>warning: [client][option][deprecated]</dt>
<dd>非推奨のオプションを指定した。</dd>
<dt>statistics: [sessions][finished]</dt>
<dd>セッション終了時。メッセージとして以下のフォーマットの統計情報が付く。

<pre>#{総処理セッション数}(+#{前回から処理したセッション数}) #{処理中のセッション数}</pre>

例えば、前回のログ出力時には27セッション処理した状態で、現在は追加で2セッションの処理が完了し、3セッションを処理中である場合は以下のようになります。

<pre>29(+2) 3</pre></dd>
<dt>statistics: [reply][end-of-message][quarantine]</dt>
<dd>隔離レスポンスを返した。</dd></dl>

### server

serverモジュールのログの一覧です。

serverモジュールは同時に複数のmilterと通信する用途を想定しています。そのため、以下のようにログには通信相手のmilter名が含まれます。

<pre>[#{セッションID}] [#{タグ名1}][#{タグ名2}][...] [#{milter名}] #{メッセージ}</pre>

なお、clientモジュール同様、セッションに関係ない部分では「セッションID」は付きません。以下はタグ部分について

<dl>
<dt>error: [server][dispose][body][remained]</dt>
<dd>セッション終了後も未送信の本文が残っている。通信相手のmilterが本文の通信途中で強制的に接続を切った可能性がある。</dd>
<dt>error: [server][flushed][error][next-state][invalid]</dt>
<dd>非同期のデータ書き出し時に不正な状態遷移を検出した。通信相手のmilterがmilterプロトコルに沿っていない通信した可能性がある。</dd>
<dt>error: [server][error]</dt>
<dd>想定外のエラーが発生。詳細はメッセージで確認する。</dd>
<dt>error: [server][error][write]</dt>
<dd>書き込み時にエラーが発生。</dd>
<dt>error: [server][error][#{response}][state][invalid][#{state}]</dt>
<dd>状態「#{state}」の時にmilterから意図しないレスポンス「#{response}」が返ってきた。メッセージには期待するレスポンスの一覧が含まれている。通信相手のmilterがmilterプロトコルに沿っていない通信をした可能性がある。

状態「#{state}」は以下のいずれかである。

<dl>
<dt>invalid</dt>
<dd>不正な状態。通常はこの状態にならない。</dd>
<dt>start</dt>
<dd>通信を開始した直後の状態。</dd>
<dt>define-macro</dt>
<dd>マクロ定義を通信している状態。</dd>
<dt>negotiate</dt>
<dd>milterとセッション内でのやりとり方法を調整している状態。</dd>
<dt>connect</dt>
<dd>negotiateが完了し、milterとの接続が確立された状態。</dd>
<dt>helo</dt>
<dd>SMTPのHELOコマンドに対応するmilterプロトコルを処理している状態。</dd>
<dt>envelope-from</dt>
<dd>SMTPのMAIL FROMコマンドに対応するmilterプロトコルを処理している状態。</dd>
<dt>envelope-recipient</dt>
<dd>SMTPのRCPTコマンドに対応するmilterプロトコルを処理している状態。</dd>
<dt>data</dt>
<dd>SMTPのDATAコマンドに対応するmilterプロトコルを処理している状態。</dd>
<dt>unknown</dt>
<dd>SMTPで未知のコマンドに対応するmilterプロトコルを処理している状態。</dd>
<dt>header</dt>
<dd>SMTPのDATAコマンドで送信されたメールのヘッダーをmilterプロトコルで処理している状態。</dd>
<dt>end-of-header</dt>
<dd>SMTPのDATAコマンドで送信されたメールのヘッダーの処理が終わったという通知をmilterプロトコルで処理している状態。</dd>
<dt>body</dt>
<dd>SMTPのDATAコマンドで送信されたメールの本文をmilterプロトコルで処理している状態。</dd>
<dt>end-of-message</dt>
<dd>SMTPのDATAコマンドで送信されたメールの処理が終わったという通知をmilterプロトコルで処理している状態。</dd>
<dt>quit</dt>
<dd>milterプロトコルで終了処理をしている状態。</dd>
<dt>abort</dt>
<dd>milterプロトコルで中断処理をしている状態。</dd></dl>

milterからのレスポンス「#{response}」は以下のいずれかである。それぞれの項目はmilterプロトコルで使われているレスポンスに対応している。

<dl>
<dt>negotiate-reply</dt>
<dd>negotiateのレスポンス。</dd>
<dt>continue</dt>
<dd>処理の継続を表すレスポンス。</dd>
<dt>reply-code</dt>
<dd>SMTPのレスポンスコードを指定するレスポンス。</dd>
<dt>add-header</dt>
<dd>ヘッダーを末尾に追加するレスポンス。</dd>
<dt>insert-header</dt>
<dd>ヘッダーを任意の位置に挿入するレスポンス。</dd>
<dt>change-header</dt>
<dd>ヘッダーを変更するレスポンス。</dd>
<dt>add-recipient</dt>
<dd>宛先を変更するレスポンス。</dd>
<dt>delete-recipient</dt>
<dd>宛先を削除するレスポンス。</dd>
<dt>replace-body</dt>
<dd>本文を変更するレスポンス。</dd>
<dt>progress</dt>
<dd>処理が継続中であることを示すレスポンス。タイムアウト時間を伸ばすために利用される。</dd>
<dt>quarantine</dt>
<dd>メールを隔離するレスポンス。</dd>
<dt>skip</dt>
<dd>本文の受信を中止するレスポンス。</dd></dl></dd>
<dt>error: [server][timeout][connection]</dt>
<dd>接続処理がタイムアウト。</dd>
<dt>error: [server][error][connect]</dt>
<dd>接続エラーが発生。</dd>
<dt>error: [server][error][connected][start]</dt>
<dd>接続開始時の初期化処理時にエラーが発生。</dd>
<dt>warning: [server][reply][quitted][#{state}][#{response}]</dt>
<dd>milterプロトコルが終了したあとにレスポンスが返ってきた。状態「#{state}」とレスポンス「#{response}」は【[server][error][#{response}][state][invalid][#{state}]】で説明している項目と同じ。このログの次に error ログを出力していなければ問題ない。</dd></dl>

### manager

managerモジュールのログの一覧です。

managerモジュールもclientモジュール同様、セッションに関係ない部分では「セッションID」は付きません。

#### サブモジュール

一番最初のタグはmanagerモジュール内のサブモジュール名になります。ただし、以下のログについてはタグが付きません。

* クラッシュ時のログ
* milter-managerコマンド起動時に関連するログ

サブモジュールは以下の8つです。

* manager
* configuration
* launcher
* process-launcher
* controller
* leader
* children
* egg

<dl>
<dt>manager</dt>
<dd>各モジュールを利用してmilter-managerコマンドを実装しているサブモジュールです。</dd>
<dt>configuration</dt>
<dd>設定ファイルを読み込むモジュール。</dd>
<dt>launcher</dt>
<dd>子milterを起動するモジュール。</dd>
<dt>process-launcher</dt>
<dd>milter-managerコマンド内でlauncherサブモジュールを起動するモジュール。</dd>
<dt>controller</dt>
<dd>milter-managerプロセスを外部から操作するモジュール。</dd>
<dt>leader</dt>
<dd>childrenモジュールがまとめた子milter全体の処理結果をMTAに返すモジュール。</dd>
<dt>children</dt>
<dd>複数の子milterに対してmilterプロトコルで処理した結果を統合して、全体の処理結果としてまとめるモジュール。</dd>
<dt>egg</dt>
<dd>1つの子milterに関する情報を管理するモジュール。</dd></dl>

それぞれのサブモジュール毎に説明します。

#### サブモジュール: manager

<dl>
<dt>error: [manager][reload][signal][error]</dt>
<dd>SIGHUPシグナルによってリクエストされた設定の再読み込み中にエラーが発生。</dd></dl>

#### サブモジュール: configuration

<dl>
<dt>error: [configuration][dispose][clear][error]</dt>
<dd>終了時の設定クリアが失敗。</dd>
<dt>error: [configuration][new][clear][error]</dt>
<dd>初回の設定クリアが失敗。</dd>
<dt>error: [configuration][load][clear][error]</dt>
<dd>設定再読み込み前の設定クリアに失敗。</dd>
<dt>error: [configuration][load][error]</dt>
<dd>設定ファイル（milter-manager.conf）の読み込みに失敗。</dd>
<dt>error: [configuration][load][custom][error]</dt>
<dd>カスタム設定ファイル（milter-manager.custom.conf）の読み込みに失敗。</dd>
<dt>error: [configuration][clear][custom][error]</dt>
<dd>Rubyでの設定クリア処理に失敗。</dd>
<dt>error: [configuration][maintain][error]</dt>
<dd>manager.maintenance_intervalで指定した間隔で実行されるメンテナンス処理中にエラーが発生。</dd>
<dt>error: [configuration][event-loop-created][error]</dt>
<dd>イベントループの作成後の処理でエラーが発生。</dd></dl>

#### サブモジュール: launcher

<dl>
<dt>error: [launcher][error][child][authority][group]</dt>
<dd>子milterを起動する時に、指定したグループへの実行権限の変更に失敗。</dd>
<dt>error: [launcher][error][child][authority][groups]</dt>
<dd>子milterを起動する時に、指定したユーザの追加グループの初期化に失敗。</dd>
<dt>error: [launcher][error][child][authority][user]</dt>
<dd>子milterを起動する時に、指定したユーザへの実行権限の変更に失敗。</dd>
<dt>error: [launcher][error][launch]</dt>
<dd>子milterの起動に失敗。</dd>
<dt>error: [launcher][error][write]</dt>
<dd>子milterの起動が成功したか失敗したかを返すレスポンスの書き込み時にエラーが発生。</dd></dl>

#### サブモジュール: process-launcher

<dl>
<dt>error: [process-launcher][error][start]</dt>
<dd>launcherモジュールとの接続開始処理でエラーが発生。</dd>
<dt>error: [process-launcher][error]</dt>
<dd>forkしたlauncherモジュールからのファイルディスクリプタ切り離しに失敗。</dd></dl>

#### サブモジュール: controller

<dl>
<dt>error: [controller][error][write][success]</dt>
<dd>成功を示すレスポンスデータの書き込み時にエラーが発生。</dd>
<dt>error: [controller][error][write][error]</dt>
<dd>エラーを示すレスポンスデータの書き込み時にエラーが発生。</dd>
<dt>error: [controller][error][save]</dt>
<dd>設定ファイルの保存時にエラーが発生。</dd>
<dt>error: [controller][error][write][configuration]</dt>
<dd>設定ファイルの書き込み時にエラーが発生。</dd>
<dt>error: [controller][reload][error]</dt>
<dd>設定の再読み込み時にエラーが発生。</dd>
<dt>error: [controller][error][write][status]</dt>
<dd>ステータスを返すレスポンスの書き込み時にエラーが発生。</dd>
<dt>error: [controller][error][unix]</dt>
<dd>controller.remove_unix_socket_on_createを有効にした場合に終了時に実行する、使用したUNIXドメインソケットの削除処理中にエラーが発生。</dd>
<dt>error: [controller][error][start]</dt>
<dd>通信開始処理に失敗。</dd>
<dt>error: [controller][error][accept]</dt>
<dd>接続の受け付けに失敗。</dd>
<dt>error: [controller][error][watch]</dt>
<dd>通信中にエラーが発生。</dd>
<dt>error: [controller][error][listen]</dt>
<dd>listen(2)に失敗。</dd></dl>

#### サブモジュール: leader

<dl>
<dt>error: [leader][error][invalid-state]</dt>
<dd>不正な状態遷移を検出。milter managerのバグである可能性が高い。</dd>
<dt>error: [leader][error]</dt>
<dd>childrenモジュールでエラーが発生。</dd>
<dt>error: [leader][error][reply-code]</dt>
<dd>SMTPのレスポンスコード指定に失敗。通信相手のmilterが不正なレスポンスコードを送っている可能性がある。</dd>
<dt>error: [leader][error][add-header]</dt>
<dd>ヘッダーを末尾に追加する時にエラーが発生。</dd>
<dt>error: [leader][error][insert-header]</dt>
<dd>ヘッダーを任意の位置に追加する時にエラーが発生。</dd>
<dt>error: [leader][error][delete-header]</dt>
<dd>ヘッダーを削除するときにエラーが発生。</dd>
<dt>error: [leader][error][change-from]</dt>
<dd>差出人を変更するときにエラーが発生。</dd>
<dt>error: [leader][error][add-recipient]</dt>
<dd>宛先を追加する時にエラーが発生。</dd>
<dt>error: [leader][error][delete-recipient]</dt>
<dd>宛先を削除する時にエラーが発生。</dd>
<dt>error: [leader][error][replace-body]</dt>
<dd>本文を置き換える時にエラーが発生。</dd></dl>

#### サブモジュール: children

<dl>
<dt>error: [children][error][negotiate]</dt>
<dd>子milterにネゴシエーション（セッション内でのやりとり方法の調整）開始のリクエストを送っていないのにレスポンスが返ってきた。通信相手のmilterがmilterプロトコルに沿っていない通信をした可能性がある。</dd>
<dt>error: [children][error][pending-message-request]</dt>
<dd>milter-managerプロセス内部で遅延させた子milterへのリクエストに問題があった。発生した場合はmilter manager内のバグである。</dd>
<dt>error: [children][error][body][read][seek]</dt>
<dd>本文データの読み込み位置の変更時にエラーが発生。</dd>
<dt>error: [children][error][body][read]</dt>
<dd>本文データの読み込み時にエラーが発生。</dd>
<dt>error: [children][error][invalid-state][temporary-failure]</dt>
<dd>milterプロトコルでは想定していないタイミングで子milterがtemporary-failureレスポンスを返した。通信相手のmilterがmilterプロトコルに沿っていない通信をした可能性がある。</dd>
<dt>error: [children][error][invalid-state][reject]</dt>
<dd>milterプロトコルでは想定していないタイミングで子milterがrejectレスポンスを返した。通信相手のmilterがmilterプロトコルに沿っていない通信をした可能性がある。</dd>
<dt>error: [children][error][invalid-state][accept]</dt>
<dd>milterプロトコルでは想定していないタイミングで子milterがacceptレスポンスを返した。通信相手のmilterがmilterプロトコルに沿っていない通信をした可能性がある。</dd>
<dt>error: [children][error][invalid-state][discard]</dt>
<dd>milterプロトコルでは想定していないタイミングで子milterがdiscardレスポンスを返した。通信相手のmilterがmilterプロトコルに沿っていない通信をした可能性がある。</dd>
<dt>error: [children][error][invalid-state][stopped]</dt>
<dd>milterプロトコルでは想定していないタイミングでmilter-managerプロセスが子milterの実行を停止した。milter manager内のバグである可能性がある。</dd>
<dt>error: [children][error][invalid-state][#{response}][#{state}]</dt>
<dd>end-of-message時しか返せないレスポンス「#{response}」を状態「#{state}」の時に返した。通信相手のmilterがmilterプロトコルに沿っていない通信をした可能性がある。

レスポンス「#{response}」は以下のいずれかである。

<dl>
<dt>add-header</dt>
<dd>ヘッダーを末尾に追加するレスポンス。</dd>
<dt>insert-header</dt>
<dd>ヘッダーを任意の位置に追加するレスポンス。</dd>
<dt>change-header</dt>
<dd>ヘッダーを変更するレスポンス。</dd>
<dt>delete-header</dt>
<dd>ヘッダーを変更するレスポンス。</dd>
<dt>change-from</dt>
<dd>差出人を変更するレスポンス。</dd>
<dt>add-recipient</dt>
<dd>宛先を追加するレスポンス。</dd>
<dt>delete-recipient</dt>
<dd>宛先を削除するレスポンス。</dd>
<dt>replace-body</dt>
<dd>本文を変更するレスポンス。</dd>
<dt>progress</dt>
<dd>処理が継続中であることを示すレスポンス。</dd>
<dt>quarantine</dt>
<dd>メールを隔離するレスポンス。</dd></dl>

状態「#{state}」は【[server][error][#{response}][state][invalid][#{state}]】で説明している項目と同じ。</dd>
<dt>error: [children][timeout][writing]</dt>
<dd>書き込みがタイムアウト。</dd>
<dt>error: [children][timeout][reading]</dt>
<dd>読み込みがタイムアウト。</dd>
<dt>error: [children][timeout][end-of-message]</dt>
<dd>end-of-message時に指定した時間内にレスポンスが返ってこなかった。</dd>
<dt>error: [children][error][#{state}][#{fallback_status}]</dt>
<dd>子milterとの通信に利用しているserverモジュールでエラーが発生。子milterの処理結果はmilter.fallback_statusで指定したステータス「#{fallback_status}」を利用。

状態「#{state}」は【[server][error][#{response}][state][invalid][#{state}]】で説明している項目と同じ。</dd>
<dt>error: [children][error][start-child][write]</dt>
<dd>launcherサブモジュールへの子milterの起動リクエストの書き込み時にエラーが発生。</dd>
<dt>error: [children][error][start-child][flush]</dt>
<dd>launcherサブモジュールへの子milterの起動リクエストのフラッシュ時にエラーが発生。</dd>
<dt>error: [children][error][negotiate][not-started]</dt>
<dd>MTAがネゴシエーション（セッション内でのやりとり方法の調整）を開始していないのに、ネゴシエーションのレスポンス処理を実行。milter manager内のバグである可能性がある。</dd>
<dt>error: [children][error][negotiate][no-response]</dt>
<dd>すべての子milterからネゴシエーション（セッション内でのやりとり方法の調整）のレスポンスがない。子milterの問題である可能性が高い。</dd>
<dt>error: [children][timeout][connection]</dt>
<dd>指定した時間内に接続を確立できなかった。</dd>
<dt>error: [children][error][connection]</dt>
<dd>接続処理時にエラーが発生。</dd>
<dt>error: [children][error][alive]</dt>
<dd>全ての子milterが終了しているのにまだMTAからリクエストがきている。</dd>
<dt>error: [children][error][message-processing]</dt>
<dd>SMTPのDATAコマンドで送信されたメールの処理を実行している子milterがいないのに、MTAからメール処理のリクエストがきた。</dd>
<dt>error: [children][error][body][open]</dt>
<dd>大きなメール本文保存用の一時ファイルの作成に失敗。</dd>
<dt>error: [children][error][body][encoding]</dt>
<dd>大きなメール本文保存用の一時ファイルの読み書き用のエンコーディングの設定に失敗。</dd>
<dt>error: [children][error][body][write]</dt>
<dd>大きなメール本文保存用の一時ファイルへの本文書き込みに失敗。</dd>
<dt>error: [children][error][body][send][seek]</dt>
<dd>大きなメール本文保存用の一時ファイルの読み出し位置指定に失敗。</dd>
<dt>error: [children][error][body][send]</dt>
<dd>大きなメール本文保存用の一時ファイルからの本文読み出しに失敗。</dd></dl>

#### サブモジュール: egg

<dl>
<dt>error: [egg][error]</dt>
<dd>子milter接続時に不正な接続情報が指定されていることを検出。</dd>
<dt>error: [egg][error][set-spec]</dt>
<dd>子milterに不正な接続情報を指定。</dd></dl>

#### その他: クラッシュ時のログ

<dl>
<dt>critical: [#{signal}] unable to open pipe for collecting stack trace</dt>
<dd>クラッシュ時にスタックトレース取得用のパイプの作成に失敗した。シグナル「#{signal}」は「SEGV」または「ABORT」になる。</dd>
<dt>critical: #{stack_trace}</dt>
<dd>タグはつかない。スタックトレースの各行を表示する。</dd></dl>

#### その他: milter-managerコマンド起動時のログ

<dl>
<dt>error: failed to create pipe for launcher command</dt>
<dd>launcherモジュール用プロセスへのコマンド送信用パイプ作成に失敗。</dd>
<dt>error: failed to create pipe for launcher reply</dt>
<dd>launcherモジュール用プロセスからのレスポンス受信用パイプ作成に失敗。</dd>
<dt>error: failed to fork process launcher process</dt>
<dd>launcherモジュール用プロセスのforkに失敗。</dd>
<dt>error: failed to find password entry for effective user</dt>
<dd>指定した実行ユーザが見つからない。</dd>
<dt>error: failed to get password entry for effective user</dt>
<dd>指定した実行ユーザの取得に失敗。</dd>
<dt>error: failed to get limit for RLIMIT_NOFILE</dt>
<dd>開けるファイルディスクリプタ数の制限値取得に失敗。</dd>
<dt>error: failed to set limit for RLIMIT_NOFILE</dt>
<dd>開けるファイルディスクリプタ数の制限値設定に失敗。</dd>
<dt>error: failed to create custom configuration directory</dt>
<dd>カスタム設定ディレクトリの作成に失敗。</dd>
<dt>error: failed to change owner and group of configuration directory</dt>
<dd>設定ディレクトリの所有者・グループの変更に失敗。</dd>
<dt>error: failed to listen</dt>
<dd>listen(2)に失敗。</dd>
<dt>error: failed to drop privilege</dt>
<dd>root権限を落とすことに失敗。</dd>
<dt>error: failed to listen controller socket:</dt>
<dd>controllerモジュールがlisten(2)に失敗。</dd>
<dt>error: failed to daemonize:</dt>
<dd>デーモン化に失敗。</dd>
<dt>error: failed to start milter-manager process:</dt>
<dd>milter-managerプロセスの起動処理に失敗。</dd>
<dt>error: [manager][reload][custom-load-path][error]</dt>
<dd>設定の再読込に失敗。</dd>
<dt>error: [manager][configuration][reload][command-line-load-path][error]</dt>
<dd>コマンドラインで指定したロードパスからの設定の読み込みに失敗。</dd></dl>

#### その他: 統計情報

<dl>
<dt>statistics: [milter][header][add]</dt>
<dd>子milterがヘッダを追加。</dd>
<dt>statistics: [milter][end][#{last_state}][#{status}][#{elapsed}]</dt>
<dd>子milterが1つ終了。

「[#{last_state}]」には終了時の状態が入る。「[#{last_state}]」に入る値は【[server][error][#{response}][state][invalid][#{state}]】の「#{state}」と同じである。

「[#{status}]」には子milterからのレスポンスが入る。レスポンスは以下のいずれかがである。

<dl>
<dt>reject</dt>
<dd>拒否を表すレスポンス。</dd>
<dt>discard</dt>
<dd>廃棄を表すレスポンス。</dd>
<dt>accept</dt>
<dd>受信を表すレスポンス。</dd>
<dt>temporary-failure</dt>
<dd>一時拒否を表すレスポンス。</dd>
<dt>pass</dt>
<dd>暗黙的な受信を表すレスポンス。</dd>
<dt>stop</dt>
<dd>処理を途中で中断したことを表すレスポンス。</dd></dl>

「[#{elapsed}]」には接続してからの経過時間が入る。</dd>
<dt>statistics: [session][end][#{last_state}][#{status}][#{elapsed}]</dt>
<dd>milterセッションが1つ終了。「[#{last_state}]」、「#{status}」、「#{elapsed}」に入る値は【[milter][end][#{last_state}][#{status}][#{elapsed}]】と同様。</dd>
<dt>statistics: [session][disconnected][#{elapsed}]</dt>
<dd>SMTPクライアントがセッションを途中で切断。

「[#{elapsed}]」には接続してからの経過時間が入る。</dd>
<dt>statistics: [session][header][add]</dt>
<dd>milterセッション全体のレスポンスとしてヘッダーを追加。</dd></dl>


