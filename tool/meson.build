# Copyright (C) 2022  Sutou Kouhei <kou@clear-code.com>
#
# This library is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with this library.  If not, see <http://www.gnu.org/licenses/>.

executable(
    'milter-test-client-libmilter',
    'milter-test-client-libmilter.c',
    c_args: '-DMILTER_LOG_DOMAIN="milter-test-client-libmilter"',
    dependencies: [config, dependency('glib-2.0')],
    install: true,
    link_with: libmilter,
)
executable(
    'milter-test-client',
    'milter-test-client.c',
    c_args: '-DMILTER_LOG_DOMAIN="milter-test-client"',
    dependencies: [milter_client],
    install: true,
)
executable(
    'milter-test-server',
    'milter-test-server.c',
    c_args: '-DMILTER_LOG_DOMAIN="milter-test-server"',
    dependencies: [milter_server],
    install: true,
)
