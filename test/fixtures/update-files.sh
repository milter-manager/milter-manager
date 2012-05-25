#!/bin/sh

list_paths()
{
    variable_name=$1
    echo "$variable_name = \\"
    sort | \
    sed \
      -e 's,^,\t,' \
      -e 's,$, \\,'
    echo "\t\$(NULL)"
    echo
}

find "." -type f -name "*.txt" -or -name "*.conf" | \
    sed -e 's,^\./,,g' | \
    list_paths "EXTRA_DIST"
