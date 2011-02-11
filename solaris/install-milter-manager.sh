#! /usr/bin/bash --noprofile

set -e

source ./environment.sh
source ./functions.sh

install_milter_manager()
{
    local log="${BUILDS}/milter-manager.build.log"

    echo "$(time_stamp): Configuring milter-manager..."
    (
        cd "${base_dir}/../"
        run bash ./autogen.sh
        run ./configure --prefix $PREFIX \
	    --enable-ruby-milter \
	    --with-package-platform=solaris
    ) > "${log}"
    echo "$(time_stamp): done."

    install_package_build_and_install "milter-manager" "${base_dir}/../"
}

echo "$(time_stamp): Installing milter manager..."
install_milter_manager
echo "$(time_stamp): done."
