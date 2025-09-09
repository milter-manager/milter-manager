#!/bin/bash
#
# Copyright (C) 2021-2022  Sutou Kouhei <kou@clear-code.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

set -eux

cache_dir=/host/.cache/ccache/$(. /etc/os-release && echo "${ID}-${VERSION_ID}")
if mkdir -p ${cache_dir}; then
  export CCACHE_DIR=${cache_dir}
fi

cd /build
../host/configure \
  --with-default-connection-spec="inet:10025@[127.0.0.1]"
make -j$(nproc)
# Ensure creating src/.libs/lt-milter-manager
src/milter-manager --version >/dev/null 2>&1 || :

../host/binding/ruby/test/run-test.sh -vv
../host/test/run-test.sh
../host/test/tool/run-test.sh
