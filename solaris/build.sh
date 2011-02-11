#!/usr/bin/bash --noprofile

source ./functions.sh

run ./build-libraries.sh
run ./build-milter-manager.sh
run ./build-pkgs.sh
