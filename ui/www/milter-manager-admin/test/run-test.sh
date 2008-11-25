#!/bin/sh

base_dir=`dirname $0`
top_dir="$base_dir/.."

(cd $top_dir; TESTOPTS="$@" rake -s test:all)
