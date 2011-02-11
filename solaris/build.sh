#!/usr/bin/bash --noprofile

source ./functions.sh

run ./install-libraries.sh
run ./install-milter-manager.sh
run ./build-pkgs.sh
