#!/usr/bin/bash --noprofile

set -e

source ./environment.sh
source ./functions.sh

echo "$(time_stamp): Updating prototypes..."

for path in $(ls "${BUILDS}"); do
    if [ -d "${path}" ]; then
	update_prototype "${path##*/}" "${path}"
    fi
done

install_package "milter-manager" "${base_dir}/.."

echo "$(time_stamp): done."
