---
title: Reverse Dictionary
---

# Reverse Dictionary --- Find how to do by want to do

## About this document

This is a document to find how to do by want to do as key.

## Install

### Install to Debian GNU/Linux

Install to Debian GNU/Linuxdescribes it. You can maintain milter manager package easilybecause milter manager can be installed with aptitude.

### Install to Ubuntu Linux

Install to Ubuntu Linux describesit. You can maintain milter manager package easily becausemilter manager can be installed with aptitude.

### Install to CentOS

Install to CentOS describes it.You can maintain milter manager package easily becausemilter manager can be installed as RPM package.

### Install to FreeBSD

Install to FreeBSD describes it.

### Generate graphs for statistics data

There are documents for each platform:

* For Debian
* For Ubuntu
* For CentOS
* For FreeBSD

## Configuration: Basic

### Find a configuration file {#configuration-basic-find-configuration-file}

Here are configuration file locations when you installmilter manager followed by install manual for your platform:

* Ubuntu: /etc/milter-manager/milter-manager.local.conf
* CentOS: /etc/milter-manager/milter-manager.local.conf
* Ubuntu: /usr/local/etc/milter-manager/milter-manager.local.conf

milter-manager.local.conf is a file what you createnewly. milter-manager.conf loads milter-manager.local.confin the same directory by default.

### Connect milter-manager via TCP/IP

"inet:" is used for manager.connection_spec.

<pre># Listen on 10025 port. milter-manager accepts a connection from localhost
manager.connection_spec = "inet:10025@localhost"</pre>

### Connect milter-manager via UNIX domain socket

"unix:" is used for manager.connection_spec.

<pre># Listen on /var/run/milter/milter-manager.sock.
manager.connection_spec = "unix://var/run/milter/milter-manager.sock"</pre>

A socket file permission can be specified bymanager.unix_socket_mode.

<pre># Users who belongs to the same group that owns the socket
# can connect to milter-manager.
manager.unix_socket_mode = 0660</pre>

A group for socket file can be specified bymanager.unix_socket_group.

<pre># Socket file is belongs to "milter" group.
manager.unix_socket_group = "milter"</pre>

### Cleanup UNIX domain socket

Creating socket is failed when the same name of newly UNIXdomain socket. To avoid the situation, milter-managerprovides features that remove socket file on the followingpoints:

1. before creating a UNIX domain socket
2. after finishing a UNIX domain socket use

Normally, milter-manager doesn't fail to create a socket by'socket file already exists' reason because milter-managerenables both of them by default.

If you want to disable the features, change the followingconfiguration respectively:

1. manager.remove_unix_socket_on_create
2. manager.remove_unix_socket_on_close

Here is an configuration to disable both of them:

<pre># Don't remove an existing socket file before creating a socket file
manager.remove_unix_socket_on_create = false
# Don't remove a socket file after its use
manager.remove_unix_socket_on_close = false</pre>

## Configuration: Application

### Apply milters to messages only for specified accounts

milter-manager provides a sample configuration to restrictmilter application to specified account. This section showsan example that all registered milters only applied to thefollowing accounts:

1. test-user@example.com
2. all accounts in test.example.com domain

Here is a configuration to be appended tomilter-manager.local.conf:

<pre>restrict_accounts_by_list("test-user@example.com",
                          /@test\.example\.com\z/)</pre>

This configuration syntax may be changed because this isstill sample. But a feature provided by this configurationwill be still provided. The feature will be more powerful bysupporting database and LDAP as an account source in thefuture.


