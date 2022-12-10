---
title: その他のプラットフォームへインストール
---

# その他のプラットフォームへインストール --- その他のプラットフォームへのmilter managerのインストール方法

## このドキュメントについて

Ubuntu Linux、CentOS、FreeBSD以外のプラットフォームにmilter managerをインストールときのヒントを示します。プラットフォームに依存しない一般的なインストール情報はインストールを見てください。

## パッケージプラットフォームの検出

milter-managerはシステムにインストールされているmilterを自動検出します。どのように検出するかはconfigure時に判断します。configureの最後に出力される"Package Platform"が判断した検出方法です。

<pre>% ./configure
...
Configure Result:

  Package Platform : debian
  Package Options  :
...</pre>

この例だと"debian"系のプラットフォーム用の検出方法を用います。

"unknown"となっている場合は、configureが判断に失敗したか、milter-managerがサポートしていないプラットフォームかのどちらかです。

もし、configureが判断に失敗しているのであれば、明示的に"--with-package-platform"を指定することができます。

<pre>% ./configure --with-package-platform=freebsd
...
Configure Result:

  Package Platform : freebsd
  Package Options  :
...</pre>

また、--with-package-optionsを指定することで、付加情報を指定することができます。付加情報は「名前1=値1,名前2=値2,...」というように指定します。例えば、pkgsrcプラットフォームで起動スクリプトを/usr/pkg/etc/rc.d/以下ではなく、/etc/rc.d/以下にインストールしている場合は以下のようになります。

<pre>% ./configure --with-package-platform=pkgsrc --with-package-options=prefix=/etc
...
Configure Result:

  Package Platform : pkgsrc
  Package Options  : prefix=/etc
...</pre>

pkgsrcのために、特別に--with-rcddirオプションが用意してあります。このオプションを使うと、上述の設定は以下のようなオプションになります。

<pre>% ./configure --with-package-platform=pkgsrc --with-rcddir=/etc/rc.d
...
Configure Result:

  Package Platform : pkgsrc
  Package Options  : rcddir=/etc/rc.d
...</pre>

現在サポートしているプラットフォームはdebian, redhat,freebsd(, pkgsrc)ですが、独自のプラットフォームを指定することもできます。

例えば、新しくsuseプラットフォームに対応するとします。

<pre>% ./configure --with-package-platform=suse
...
Configure Result:

  Package Platform : suse
  Package Options  :
...</pre>

これでsuseプラットフォームを使用することになりました。後は、suseプラットフォーム用のmilter自動検出スクリプトをRubyで書くだけです。スクリプトは$prefix/etc/milter-manager/defaults/以下に"#{プラットフォーム名}.conf"という名前で作成します。今の例でいうと"suse.conf"になります。

$prefix/etc/milter-manager/defaults/suse.confの中では、milterを検出し、define_milterで検出したmilterを登録します。milterの登録方法の詳細は設定を見てください。

また、インストール後に現在のプラットフォームを確認する場合は、以下のようにします。

<pre>% /usr/local/sbin/milter-manager --show-config
...
package.platform = "debian"
package.options = nil
...</pre>

この場合は、プラットフォームはdebianで、付加情報は指定されていないということになります。


