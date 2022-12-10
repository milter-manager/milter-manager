---
title: Upgrade on FreeBSD
---

# Upgrade on FreeBSD --- How to upgrade milter manager on FreeBSD

## About this document

This document describes how to upgrade milter manager onFreeBSD. See Install to FreeBSDfor newly install information.

## Upgrade

We can upgrade milter manager by overriding existinginstallation.

NOTE: /usr/local/etc/milter-manager/milter-manager.conf isoverridden. If you changed it, please backup it beforeupgrading. If you are using milter-manager.local.conf notmilter-manager.conf, you don't need to backupmilter-manager.conf.

<pre>% sudo pkg update
% sudo pkg upgrade --yes milter-manager</pre>

## Conclusion

milter manager can be upgraded easily. It means that miltermanager is a low maintenance cost software.

If we want to use improvements in a new version, pleaseupgrade your milter manager.

If we've also installed optional packages, we will go toUpgrade on FreeBSD(optional).


