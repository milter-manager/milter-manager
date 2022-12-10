---
title: Ruby製milterの設定
---

# Ruby製milterの設定 --- 設定ファイルの書き方

## このドキュメントについて

Rubyで作成したmilterはmilter-managerと同じ書式の設定ファイルをサポートしています。ただし、書式は同じですが設定項目は違います。この文書ではRubyで作成したmilterの設定ファイルの書き方と設定項目について説明します。

## 概要

設定ファイルは-cまたは--configurationオプションで指定します。例えば、milter-regexp.rbというmilterで/etc/milter-regexp.confという設定ファイルを読み込む場合は以下のように指定します。

<pre>% ruby milter-regexp.rb -c /etc/milter-regexp.conf</pre>

設定ファイルは以下のような書式になります。

<pre>グループ名.項目名 = 値</pre>

例えば、milterが接続を受け付けるソケットのアドレスをIPv4の12345番ポートに設定する場合は、以下のように「milter」グループの「connection_spec」項目の値を「"inet:12345"」にします。

<pre>milter.connection_spec = "inet:12345"</pre>

設定項目は以下のように分類されています。

* セキュリティ関連
* ログ関連
* milter関連
* データベース関連

それでは、それぞれの分類毎に説明します。

## セキュリティ関連 {#security}

セキュリティの設定はmilter-managerと同じ以下の項目が利用可能です。指定方法の説明はmilter-managerの説明ページを参照してください。

<dl>
<dt>security.effective_user</dt>
<dd>security.effective_userと同じ。</dd>
<dt>security.effective_group</dt>
<dd>security.effective_groupと同じ。</dd></dl>

## ログ関連 {#log}

ログの設定はmilter-managerと同じ以下の項目が利用可能です。指定方法の説明はmilter-managerの説明ページを参照してください。

<dl>
<dt>log.level</dt>
<dd>log.levelと同じ。</dd>
<dt>log.path</dt>
<dd>log.pathと同じ。</dd>
<dt>log.use_syslog</dt>
<dd>log.use_syslogと同じ。</dd>
<dt>log.syslog_facility</dt>
<dd>log.syslog_facilityと同じ。</dd></dl>

## milter関連 {#milter}

milterの設定もmilter-managerの「manager」グループの設定と同様です。

<dl>
<dt>milter.connection_spec</dt>
<dd>manager.connection_specと同じ。</dd>
<dt>milter.unix_socket_mode</dt>
<dd>manager.unix_socket_modeと同じ。</dd>
<dt>milter.unix_socket_group</dt>
<dd>manager.unix_socket_groupと同じ。</dd>
<dt>milter.remove_unix_socket_on_create</dt>
<dd>manager.remove_unix_socket_on_createと同じ。</dd>
<dt>milter.remove_unix_socket_on_close</dt>
<dd>manager.remove_unix_socket_on_closeと同じ。</dd>
<dt>milter.daemon</dt>
<dd>manager.daemonと同じ。</dd>
<dt>milter.pid_file</dt>
<dd>manager.pid_fileと同じ。</dd>
<dt>milter.maintenance_interval</dt>
<dd>manager.maintenance_intervalと同じ。</dd>
<dt>milter.suspend_time_on_unacceptable</dt>
<dd>manager.suspend_time_on_unacceptableと同じ。</dd>
<dt>milter.max_connections</dt>
<dd>manager.max_connectionsと同じ。</dd>
<dt>milter.max_file_descriptors</dt>
<dd>manager.max_file_descriptorsと同じ。</dd>
<dt>milter.fallback_status</dt>
<dd>manager.fallback_statusと同じ。</dd>
<dt>milter.event_loop_backend</dt>
<dd>manager.event_loop_backendと同じ。</dd>
<dt>milter.n_workers</dt>
<dd>manager.n_workersと同じ。</dd>
<dt>milter.packet_buffer_size</dt>
<dd>manager.packet_buffer_sizeと同じ。</dd>
<dt>milter.max_pending_finished_sessions</dt>
<dd>manager.max_pending_finished_sessionsと同じ。</dd>
<dt>milter.maintained</dt>
<dd>manager.maintainedと同じ。</dd>
<dt>milter.event_loop_created</dt>
<dd>manager.event_loop_createdと同じ。</dd>
<dt>milter.name</dt>
<dd>子milterの名前を取得します。1.8.1から利用可能。</dd></dl>

## データベース関連 {#database}

データベースの設定もmilter-managerの「database」グループの設定と同様です。セットアップ方法や簡単なチュートリアルはmilter-managerのドキュメントを参照してください。

<dl>
<dt>database.type</dt>
<dd>database.typeと同じ。</dd>
<dt>database.name</dt>
<dd>database.nameと同じ。</dd>
<dt>database.host</dt>
<dd>database.hostと同じ。</dd>
<dt>database.port</dt>
<dd>database.portと同じ。</dd>
<dt>database.path</dt>
<dd>database.pathと同じ。</dd>
<dt>database.user</dt>
<dd>database.userと同じ。</dd>
<dt>database.password</dt>
<dd>database.passwordと同じ。</dd>
<dt>database.setup</dt>
<dd>database.setupと同じ。</dd>
<dt>database.load_models(path)</dt>
<dd>database.load_modelsと同じ。</dd></dl>


