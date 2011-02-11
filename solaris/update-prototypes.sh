#!/usr/bin/bash --noprofile

set -e

source ./environment.sh
source ./functions.sh

echo "$(time_stamp): Updating prototypes..."

for path in $(ls "${BUILDS}"); do
    if [ -d "${BUILDS}/${path}" ]; then
	update_prototype "${path##*/}" "${BUILDS}/${path}"
    fi
done

update_prototype "milter-manager" "${base_dir}/.."

echo "$(time_stamp): done."
