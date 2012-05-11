#!/bin/sh

script_base_dir=`dirname $0`
key_id=`$script_base_dir/gpg-uid.sh`

if ! gpg --fingerprint $key_id > /dev/null; then
    gpg --keyserver pgp.mit.edu --recv-keys $key_id
fi

gpg -a --export $key_id
