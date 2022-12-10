---
title: リリース手順
---

# リリース手順

## テストの実行

以降の作業をする前にテストを実行する。rdtool, ruby-gettext, rr が必要なので無ければ入れておく。osdn_user_name には osdn.net のアカウント名を指定する。

rbenv を使用している場合は、以下のように ruby コマンドと rd2 コマンドのパスを指定する。rbenv を使用していない場合は各コマンドのパスを自動検出するようになっている。

rbenvでRubyを入れる場合は、RUBY_CONFIGURE_OPTS="--enable-shared" を指定してインストールしておく必要がある。

例:

<pre>% RUBY_CONFIGURE_OPTS="--enable-shared" rbenv install 2.2.3</pre>

TODO: 手順を整理する。NOTE: apt で入れるパッケージと gem で入れるパッケージが被っている。。。

<pre>% sudo apt install \
  ruby-gnome2-dev \
  ruby-glib2 \
  cutter-glib-support \
  cutter-testing-framework-bin \
  lcov \
  intltool \
  gtk-doc-tools \
  gnome-doc-utils \
  libglib2.0-dev \
  build-essential \
  rdtool \
  ruby-fast-gettext \
  inkscape \
  rrdtool
% gem install rdtool rttool gettext fast_gettext mechanize test-unit test-unit-rr osdn-cli octokit
% ACLOCAL_OPTIONS="-I $HOME/opt/cutter/share/aclocal" ./autogen.sh --no-update
% ./configure --prefix=/tmp/local \
  --enable-ruby-milter \
  --with-bundled-ruby-glib2 \
  --enable-gtk-doc --enable-coverage \
  --with-osdn-user=osdn_user_name \
  --with-launchpad-uploader-pgp-key=KEY \
  --with-cutter-source-path=/path/to/cutter/source \
  --with-ruby=$(rbenv which ruby) \
  --with-rd2=$(rbenv which rd2) \
  PKG_CONFIG_PATH=$HOME/opt/cutter/lib/pkgconfig
% make
% make check
% make coverage</pre>

--with-launchpad-uploader-pgp-key=KEY には長いキーを指定する。

make check でテストが失敗したり make coverage で極端にカバレッジが低い場合は何かがおかしいのでチェックする。

## 変更点の記述

変更点を NEWS, NEWS.ja に記述する。

git tag で前回リリースのタグを確認してから、コマンドを実行してログを読む。

<pre>% git log --reverse -p <前回リリースのタグ>..</pre>

例:

<pre>% git log --reverse -p $(git tag | tail -n 1)..</pre>

diff が長い場合は "Author:" で検索すると次のエントリにジャンプできる。

NEWS にログを抜粋してざっくりカテゴリ分けしてから NEWS.ja に日本語で書いたあと、NEWS の英語をチェックする。

カテゴリは以下のようにディレクトリ名やコマンド名に対応している。

<dl>
<dt>全体</dt>
<dd>全体的な修正。</dd>
<dt>milter manager</dt>
<dd>milter manager に関する修正。</dd>
<dt>milter-core</dt>
<dd>milter/core 以下の修正。</dd>
<dt>milter-client</dt>
<dd>milter/client 以下の修正。</dd>
<dt>milter-server</dt>
<dd>milter/server 以下の修正。</dd>
<dt>Ruby milter</dt>
<dd>binding/ruby/{ext,lib} 以下の修正。</dd>
<dt>コマンド名</dt>
<dd>コマンド名のコマンドの修正。例えば、milter-test-server など。</dd>
<dt>ドキュメント</dt>
<dd>ドキュメントの修正。</dd>
<dt>その他</dt>
<dd>上のカテゴリにあてはまらないその他の修正。</dd></dl>

前のバージョンのエントリを参考に

* 改良
* 修正
* 報告者や貢献者の名前

を書く。

なお、内部的な修正は基本的には NEWS には書かないが、パッチをもらったり、報告をもらったりしたものは NEWS に書くようにする。

## ドキュメントの確認と更新

新バージョンに対応したドキュメントになっているか確認して更新する。

NEWS に書いた改良点や修正点を見ながらドキュメントをチェックして、更新できていない場合は、更新する。

<pre>% make update-files
% make update-po
% make -C html
% make -C doc/reference</pre>

## バージョン情報の更新

各種パッケージやドキュメントに埋め込むバージョン情報を更新する。トップディレクトリでコマンドを実行する。OLD_RELEASE_DATE は debian/changelog をチェックすればわかる。

<pre>% make update-latest-release \
  OLD_RELEASE=$(git tag | tail -n 1) \
  OLD_RELEASE_DATE=$(grep -E "^ --" package/debian/changelog | head -n 1 | ruby -rdate -e "puts Date.parse(ARGF.read).strftime('%Y-%m-%d')") \
  NEW_RELEASE_DATE=yyyy-mm-dd
% make dist</pre>

バージョン情報を更新したら、差分を確認してからコミットする。

## リリース用パッケージ作成のための事前準備

初回のみ必要な作業である。

### OSDN への SSH アクセス

公開鍵認証でアクセスするので設定が必要である。各自、公開鍵を登録しプロジェクトへの参加申請をosdn.net上で行う。

shell.osdn.net へ SSH で接続できればよい。

% ssh shell.osdn.net(ユーザー名)@sf-usr-shell:~$ groupsusers milter-manager

のように milter-manager グループに登録されていることを確認する。

できない場合はプロジェクト管理者に相談する。

### 必須パッケージのインストール

<pre>% sudo apt install -y debootstrap gnupg pbuilder ubuntu-archive-keyring
% sudo apt install -y rinse createrepo rpm</pre>

### GPG 鍵ペアの作成

GPG 鍵ペアを持っていない場合は、作成する。以下のコマンドを実行すると、対話的に鍵ペアを作成できる。

<pre>% gpg --gen-key</pre>

作成したらキーサーバに公開鍵を送信する。

<pre>% gpg --keyserver pgp.mit.edu --send-keys <key-id></pre>

### milter manager リリース用鍵

milter-manager のリリース用鍵の情報を暗号化しておく。UID の公開鍵があれば、UID 用に暗号化できる。

<pre>% gpg -e -a -r <UID> secret.txt</pre>

復号する。

<pre>% gpg -d secret.txt.asc > secret.txt</pre>

インポートする。(secret.txt にはパスフレーズと秘密鍵が含まれている。)

<pre>% gpg --keyserver pgp.mit.edu --recv-keys 3F09D1EA68E5F18B4EC8FEEAFF2030C057B9884E
% gpg --allow-secret-key-import --import secret.txt</pre>

### リリース用gemパッケージのインストール

リリース用パッケージのリポジトリは https://packagecloud.io にて提供するようにしています。ローカルでビルドしたdebやrpmパッケージをアップロードするのには package_cloud gemを利用します。

<pre>% gem install package_cloud</pre>

パッケージのアップロードには https://packagecloud.io/milter-manager/ への権限が必要です。https://packagecloud.io/milter-manager/repos/edit から Collaborator としてユーザーを追加してもらうようにしてください。

ログインしていない状態で以下のコマンドを実行するとアクセストークンが ~/.packagecloud に保存されます。

<pre>% package_cloud repository list</pre>

## リリース用パッケージ作成

リリース用パッケージの作成には HDD の空き容量が 20GB 以上必要である。また、パッケージ作成中に sudo コマンドを使用するので事前に

<pre>% sudo ls</pre>

などで認証情報をキャッシュするようにするか NOPASSWD を設定しておく。

なお、パッケージの作成は Debian GNU/Linux か Ubuntu でしかできない。

### Debian 系

apt ディレクトリに移動してからコマンドを実行すると、一連のリリース作業を実行するが、途中で失敗することもあるので、順番に作業した方がよい。

<pre>% make release</pre>

順番に作業する場合は以下のようにする。実行するコマンドは Makefile.am に書いてあるので head などで確認する。

<pre>% make build USE_TMPFS=yes SKIP_TEST=yes
% make upload</pre>

make build に PARALLEL=yes を付けるとビルドが並列に走る。make build に USE_TMPFS=yes を付けると tmpfs を使用して高速にビルドする。PARALLEL=yes と USE_TMPFS=yes を両方付けると tmpfs が足りなくなる可能性がある。make build に SKIP_TEST=yes を付けるとBuild-Dependsからcutter関連を削除し、テストを実行しない。

初めて実行するときは、chroot 環境を作るときにロケールなどを聞かれるのでPARALLEL=yes をつけてはいけない。

TODO: 確認方法の更新make sign したら sourceforge.net をエミュレートするウェブサーバをローカルに構築してパッケージの新規インストールと更新をテストする。

Debianへインストール,Debianで更新,に書いてある手順でパッケージのインストールと更新ができることを確認する。

make upload したらウェブブラウザでhttps://packagecloud.io/milter-manager/repos?filter=debs,にアクセスしてパッケージがアップロードできていることを確認する。

sid については packagecloud が対応していないためアップロードできない。これについては、速やかに ITP を成功させて解決する方向に進めたい。

### Ubuntu

[Launchpad](https://launchpad.net/~milter-manager)でパッケージを配布する。

<pre>% cd package/ubuntu
% make upload</pre>

Ubuntuへインストール,Ubuntuで更新に書いてある手順でパッケージのインストールと更新ができることを確認する。

### RedHat 系

yum ディレクトリに移動してからコマンドを実行すると、一連のリリース作業を実行するが、途中で失敗することもあるので順番に作業した方がよい。

<pre>% make release</pre>

順番に作業する場合は以下のようにする。実行するコマンドは Makefile.am に書いてあるので head などで確認する。

<pre>% make build PARALLEL=yes
% make upload</pre>

make build に PARALLEL=yes を付けるとビルドが並列に走る。

TODO: 確認方法の更新make sign したら sourceforge.net をエミュレートするウェブサーバをローカルに構築してパッケージの新規インストールと更新をテストする。

CentOSへインストール,CentOSで更新,に書いてある手順でパッケージのインストールと更新ができることを確認する。

make upload したらウェブブラウザでhttps://packagecloud.io/milter-manager/repos?filter=rpmsにアクセスしてパッケージがアップロードできていることを確認する。

アップロード直後はインデックス更新中となるのでしばらく待ちます。

## ソースアーカイブのアップロード

ソースアーカイブの作成はもうできているのでアップロードする。トップディレクトリでコマンドを実行する。

OSDNのアクセストークンは事前に更新しておく必要がある。以下のコマンドを実行してアクセストークンを更新しておく。

<pre>% osdn login

% make release</pre>

ウェブブラウザでhttps://osdn.net/projects/milter-manager/releases/にアクセスして新しいバージョンのファイルが追加されていることを確認する。

## サイトの更新

ドキュメントをアップロードする。トップディレクトリでコマンドを実行する。

<pre>% make upload-doc
% make upload-coverage</pre>

ウェブブラウザで http://milter-manager.osdn.jp/ にアクセスして新しいドキュメントがアップロードできていることを確認する。

## リモートリポジトリにタグを打つ

トップディレクトリでコマンドを実行する。

<pre>% make tag VERSION=<version>
% git push --tags</pre>

例:

<pre>% make tag VERSION=1.8.1
% git push --tags</pre>

ウェブブラウザでhttps://github.com/milter-manager/milter-manager/tags にアクセスして新しいバージョンのタグができていることを確認する。

## GitHub にリリースを作成する

GitHub にもリリースを作成する。

タグを作成してからでないとエラーになるので、注意すること。

<pre>% make release-github VERSION=<version> ACCESS_TOKEN_FILE=<path-to-access-token-file></pre>

例:

<pre>% make release-github VERSION=2.0.4 ACCESS_TOKEN_FILE=$HOME/github-access-token.txt</pre>

## リリースメールを書いて ML に投げる

NEWS.ja, NEWS を参考にして日本語、英語のメールを書いてそれぞれ

* milter-manager-users-ja@lists.osdn.me
* milter-manager-users-en@lists.osdn.me

に投げる。

### メールのテンプレート (ja)

URL やバージョンを変更して使う。

<pre>[ANN] milter manager <new-version>

○○です。

milter manager <new-version> をリリースしました。
  https://milter-manager.osdn.jp/index.html.ja
  https://milter-manager.osdn.jp/blog/ja/

= ハイライト

このバージョンをインストールする人が「インストールしよう」と思え
るような判断材料を書く。

  * 目玉機能
  * セキュリティ
  * 致命的なバグを修正した

= インストール方法

新しくmilter managerをインストールする場合はこちらのドキュメ
ントを参考にしてください。
  https://milter-manager.osdn.jp/reference/ja/install-to.html

すでにインストールしているmilter managerをアップグレードする
場合はこちらのドキュメントを参考にしてください。
  https://milter-manager.osdn.jp/reference/ja/upgrade.html

= 変更点

<old-version>からの変更点は以下の通りです。
  https://milter-manager.osdn.jp/reference/ja/news.html#news.release-2-1-5

[ここに NEWS.ja の内容をペーストする]</pre>

### メールのテンプレート (en)

URL やバージョンを変更して使う。

<pre>[ANN] milter manager <new-version>

Hi,

milter manager <new-version> has been released.
  https://milter-manager.osdn.jp/

= Highlight

このバージョンをインストールする人が「インストールしよう」と思え
るような判断材料を書く。

  * 目玉機能
  * セキュリティ
  * 致命的なバグを修正した

= Install

Here are documents for installation:
  https://milter-manager.osdn.jp/reference/install-to.html

Here are documents for upgrading:
  https://milter-manager.osdn.jp/reference/upgrade.html

= Changes

Here are changes since <old-version>:
  https://milter-manager.osdn.jp/reference/news.html#news.release-2-1-5

[ここに NEWS の内容をペーストする]</pre>

## ブログでもリリースをアナウンスする

https://milter-manager.osdn.jp/blog/ja/

http://www.tdiary.org/ から最新版の tDiary をダウンロードしてセットアップする。

CGI で動かすよりも Rack で動かした方が便利なのでそうする。tdiary.confは milter-manager のリポジトリにあるものを使う。

<pre>% mkdir -p ~/work/ruby
% cd ~/work/ruby/
% tar xf tdiary-full-v4.2.0.tar.gz
% ln -s tdiary-v4.2.0 tdiary
% cd tdiary
% echo "gem \"tdiary-style-rd\"" >> Gemfile.local
% bundle install --path vendor/bundle
% ln -s ~/wc/milter-manager/html/blog/tdiary.conf tdiary.conf
% mkdir -p clear-code
% ln -s ~/wc/milter-manager/html/blog/clear-code clear-code/public
% bundle exec ruby bin/tdiary htpasswd

% bundle exec rackup</pre>

ウェブブラウザで http://localhost:9292/ にアクセスしてトップページが表示されればセットアップに成功している。

ウェブブラウザから新しい日記を投稿するとGitリポジトリにコミットしてプッシュまでしてくれる。(html/blog/data/ja配下) 日記投稿後、html/blog以下で次のコマンドを実行する。

<pre>% ./update.rb --osdn-user=osdn_user_name</pre>

## メジャーバージョンリリース時の追加作業

メジャーバージョンリリースというプロジェクトとしての大きなイベントを利用して、既存のmilter managerユーザーだけではなく、「名前を聞いたことがあるだけの人」や「そもそも知らなかったメール関係者」にも周知することを目的として以下の作業を実施する。

### 各種MLにアナウンス

各種MLにアナウンスするが、全く同じ文面にせず、MLごとに少しアレンジする。

また、ハイライトには前回メジャーリリースからの主な変更点や安定性が向上したなどのアピールポイントを書く。多少、大げさに書いても問題はないのでしっかりアピールする。

* 参考: 2.0.0リリースのときのリリースアナウンスのメール
  * http://sourceforge.net/mailarchive/message.php?msg_id=31226867
  * http://www.postfix-jp.info/ML/arc-2.5/msg00236.html
  * http://www.apache.jp/pipermail/spamassassin-jp/2013-July/000744.html
  * http://sourceforge.net/mailarchive/message.php?msg_id=31226848
  * http://thread.gmane.org/gmane.mail.postfix.user/238317
* 日本語ML
  * milter-manager-users-ja
  * [Postfixの日本語のメーリングリスト](http://lists.sourceforge.jp/mailman/listinfo/postfix-jp-list>)
  * [SpamAssassinの日本語のメーリングリスト](http://www.apache.jp/mailman/listinfo/spamassassin-jp)
* 英語ML
  * milter-manager-users-en
  * [Postfixのメーリングリスト postfix-users@postfix.org](http://www.postfix.org/lists.html)

### ブログに記事を書く

milter manager のブログだけでなく、ククログにも記事を書く。ククログに記事を書く場合は、その前にククログに milter manager の記事を書いた時期を考慮して milter manager の説明を最初に入れるかどうかを検討する。期間が一年以上、空いているようであれば milter manager の説明を入れるとよい。

参考: [milter manager 2.0.0 リリース - ククログ(2013-07-31)](http://www.clear-code.com/blog/2013/7/31.html)

## その他の注意

### Pbuilder + tmpfs

APTCACHEHARDLINK=no を pbuilderrc に追加しておくと以下のコマンドで使えるようになる。

<pre>$ make build USE_TMPFS=yes</pre>

### Launchpad 上でのビルドに失敗したとき

手動でバージョンを上げて再アップロードするしかない。upload.rb を修正してmake buildでやろうとすると、orig.tar.gzの内容が変わるらしくうまくいかない。一番後ろの数字を +1 する。

<pre>% dch --distribution precise --newversion 2.0.9-2~precise2 "Build for precise"
% debuild -S -sa -pgpg2 -kKEYID
% dput -f milter-manager-ppa ../milter-manager_2.0.9-2\~precise2_source.changes</pre>


