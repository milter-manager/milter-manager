#!/usr/bin/bash --noprofile

set -e

source ./environment.sh
source ./functions.sh

base_dir=$(dirname $0)
SOURCES="${base_dir}/sources"
BUILDS="${base_dir}/builds"

source ./development-sourcelist

echo done.
