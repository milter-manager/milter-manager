# -*- rd -*-
= Release procedures

== Write NEWS

You write improvements and fixes to NEWS and NEWS.ja.

You execute command "git tag" to check previous release, then you read
log.

  % git log --reverse -p 1.8.0..HEAD

If diff is too long,  you can search "Author:" to jump next entry.

  * Improvements
  * Fixes
  * Reporters and Contributers

== Check document



== Update site


== Create release packages


== Create source archive


== Create tag on remote repository



== Write release announce mails


== Write a blog entries


