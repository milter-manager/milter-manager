#!/bin/sh

script_base_dir=`dirname $0`

gpg --fingerprint > /dev/null
if test $? -nq 0; then
    gpg --keyserver pgp.mit.edu --recv-keys `$script_base_dir/gpg-uid.sh`
fi

gpg -a --export `$script_base_dir/gpg-uid.sh`
