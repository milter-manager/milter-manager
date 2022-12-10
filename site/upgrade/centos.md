---
title: Upgrade on CentOS
---

# Upgrade on CentOS --- How to upgrade milter manager on CentOS

## About this document

This document describes how to upgrade milter manager onCentOS. See Install to CentOS fornewly install information.

## Upgrade

We just upgrade milter manager package.

<pre>% sudo yum update -y milter-manager</pre>

### Upgrade from before 2.1.0

We have provided packages on[packagecloud](https://packagecloud.io/milter-manager/repos)since 2.1.0. So we need to update repository information. We canremove milter-manager-release package.

<pre>% sudo yum remove milter-manager-release</pre>

## Conclusion

milter manager can be upgraded easily. It means that miltermanager is a low maintenance cost software.

If we want to use improvements in a new version, pleaseupgrade your milter manager.

If we've also installed optional packages, we will go toUpgrade on CentOS(optional).


