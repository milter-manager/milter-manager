#!/bin/bash
# -*- indent-tabs-mode: nil; sh-basic-offset: 4; sh-indentation: 4 -*-

DISTRIBUTION=$1
CENTOS_VERSIONS=$2
PROJECT=$3
VERSION=$4
RELEASE=$5

for v in "$CENTOS_VERSIONS"; do
    find ${DISTRIBUTION} -name "*-${VERSION}-${RELEASE}*.rpm" | \
        xargs -n 1 pacakge_cloud push $PROJECT/repos/el/${v}
done
