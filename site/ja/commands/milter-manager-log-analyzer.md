---
title: milter-manager-log-analyzer / milter manager / milter managerのマニュアル
---

# milter-manager-log-analyzer / milter manager / milter managerのマニュアル

## 名前

milter-manager-log-analyzer - milter-managerのログを解析するプログラム

## 書式

<code>milter-manager-log-analyzer</code> [<em>オプション ...</em>]

## 説明

milter-manager-log-analyzerはmilter-managerのログを解析し、milter-managerや子milterの動作結果をグラフ化します。グラフは時系列で表示されるので、状況の推移を確認することができます。このため、新しい子milterを導入した前後での変化を確認する用途にも使うことができます。

## オプション

<dl>
<dt>--help</dt>
<dd>利用できるオプションを表示して終了します。</dd>
<dt>--log=LOG_FILE</dt>
<dd>LOG_FILEからログを読み込みます。

既定値は標準入力から読み込みます。</dd>
<dt>--output-directory=DIRECTORY</dt>
<dd>DIRECTORYにグラフ、HTML、グラフ生成用のデータを保存します。

既定値はカレントディレクトリ（"."）です。</dd>
<dt>--no-update-db</dt>
<dd>これまでに読み込んだデータが入っているデータベースを更新しません。グラフを出力する場合のみに有用です。

このオプションを指定しない場合は更新します。</dd></dl>

## 終了ステータス

常に0。

## 例

milter-manager-log-analyzerはcrontab内で使われるでしょう。以下はサンプルのcrontabです。

<pre>PATH=/bin:/usr/local/bin:/usr/bin
*/5 * * * * root cat /var/log/mail.info | su milter-manager -s /bin/sh -c "milter-manager-log-analyzer --output-directory ~milter-manager/public_html/log"</pre>

このサンプルでは、rootがメールログを読み込んで、milter-managerユーザ権限で動いているmilter-manager-log-analyzerに渡しています。milter-manager-log-analyzerは解析した結果を~milter-manager/public_html/log/に出力します。解析結果はhttp://localhost/~milter-manager/log/で見ることができます。

## 関連項目

milter-manager.rd.ja(1)


