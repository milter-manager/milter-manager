#!/bin/sh

script_base_dir=`dirname $0`
KEY_UID="435C1F50"

if [ $# != 1 ]; then
    echo "Usage: $0 CODE_NAMES"
    echo " e.g.: $0 'lenny hardy lucid'"
    exit 1
fi

CODE_NAMES=$1

run()
{
    "$@"
    if test $? -ne 0; then
	echo "Failed $@"
	exit 1
    fi
}

for code_name in ${CODE_NAMES}; do
    case ${code_name} in
	squeeze|wheezy|unstable)
	    distribution=debian
	    ;;
	*)
	    distribution=ubuntu
	    ;;
    esac
    for status in stable development; do
	release=${distribution}/${status}/dists/${code_name}/Release
	rm -f ${release}.gpg
	gpg --sign -ba -o ${release}.gpg -u ${KEY_UID} ${release}
    done;
done
