#!/bin/sh

script_base_dir=`dirname $0`
key_id=`$script_base_dir/gpg-uid.sh`

gpg --fingerprint > /dev/null
if test $? -nq 0; then
    gpg --keyserver pgp.mit.edu --recv-keys $key_id
fi

gpg -a --export $key_id
