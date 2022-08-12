# Copyright (C) 2022  Daijiro Fukuda <fukuda@clear-code.com>
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

sources = files(
  'libmilter-compatible.c',
)

headers = files(
  'libmilter-compatible.h',
  'mfapi.h',
  'mfdef.h',
)

install_headers(headers, subdir: project_include_sub_dir / 'libmilter' / 'libmilter')

dependencies = [
  config,
  milter_client,
]
libmilter = library('libmilter',
                    c_args: '-DMILTER_LOG_DOMAIN="libmilter"',
                    sources: sources + headers,
                    install: true,
                    dependencies: dependencies,
                    soversion: so_version,
                    version: library_version)

pkgconfig.generate(libmilter,
                   description: 'libmilter API',
                   filebase: 'libmilter',
                   name: 'Sendmail\'s libmilter compatible API by milter client library',
                   requires: ['milter-client = ' + meson.project_version()],
                   subdirs: project_include_sub_dir / 'libmilter',
                   variables: pkgconfig_variables)
