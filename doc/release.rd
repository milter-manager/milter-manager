# -*- rd -*-
= Release procedures

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

== Create release packages

=== Debian

=== RedHat

== Create source archive


== Update site

Upload documents.

== Create tag on remote repository



== Write release announce mails


== Write a blog entries


