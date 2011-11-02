# -*- rd -*-
= Release procedures

== Test

You must test before you work below.  You need to install "rdtool" and
"ruby-gettext", if you don't have those.

  % gem install rdtool gettext
  % ./autogen.sh --no-update
  % ./configure --prefix=/tmp/local \
    --enable-ruby-milter --enable-gtk-doc
  % make
  % make check

== Write NEWS

You write improvements and fixes to NEWS and NEWS.ja.

You execute command "git tag" to check previous release, then you read
log.

  % git log --reverse -p <previous release tag>..

ex(release 1.8.1):
  % git log --reverse -p 1.8.0..

If diff is too long,  you can search "Author:" to jump next entry.

Categories:

: All
  Improvements or fixes about all.
: milter manager
  Improvements or fixes about milter manager.
: milter-core
  Improvements or fixes about milter/core.
: milter-client
  Improvements or fixes about milter/client.
: milter-server
  Improvements or fixes about milter/server.
: Ruby milter
  Improvements or fixes about binding/ruby/{ext,lib}.
: command name
  Improvements or fixes about commands. Ex. milter-test-server.
: Documents
  Improvements or fixes about documents.
: Others
  Any other inmprovements or fixes.

You refer to the previous version entry and write entries below.

  * Improvements
  * Fixes
  * Reporters and Contributors

You don't have to write internal changes, but you must write changes
reported or provided a patch by someone.

== Check document

Check and update documents that are compatible with the new version.

== Update version information

You update version information. You execute below command in top
directory.

  % make dist
  % make update-latest-release \
    OLD_RELEASE=1.8.0 \
    OLD_RELEASE_DATE=yyyy-mm-dd \
    NEW_RELEASE_DATE=yyyy-mm-dd

== Prepare for creating release packages

You need this for first time.

=== Install required packages

  % sudo apt-get install -y debootstrap gnupg2
  % sudo apt-get install -y rinse createrepo rpm

=== Generate GPG key pair

...

=== Encrypt shared key for milter manager.

...

== Create release packages

Need at least 20GB of free HDD space.  You need to enable sudo-cache
before execute procedure below.

  % sudo ls

You need Debian GNU/Linux or Ubuntu to create release packages.

=== Debian

  % cd apt
  % make download
  % make build
  % make update
  % make sign
  % make upload

=== RedHat

  % cd yum
  % make download
  % make remove-existing-packages
  % make build
  % make sign
  % make update
  % make upload

== Create source archive

....

== Update site

Upload documents.

== Create tag on remote repository

You execute commands below in top directory.

  % make tag VERSION=<version>
  % git push --tags

Example:
  % make tag VERSION=1.8.1
  % git push --tags

== Write release announce mails

Write release announce in Japanese and English.

  * milter-manager-users-ja
  * milter-manager-users-en

== Write a blog entries

((<URL:http://milter-manager.sourceforge.net/blog/ja/>))


== Announce to Freecode

((<URL:http://freecode.com/projects/milter-manager>))
