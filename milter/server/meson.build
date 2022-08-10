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
  'milter-server-context.c',
  'milter-server.c',
)

headers = files(
  'milter-server-context.h',
)

enums = gnome.mkenums_simple('milter-server-enum-types',
                             body_prefix: '#include <config.h>',
                             identifier_prefix: 'Milter',
                             install_dir: milter_include_dir / 'server',
                             install_header: true,
                             sources: headers,
                             symbol_prefix: 'milter')

install_headers(headers, subdir: milter_include_sub_dir / 'server')
install_headers(['../server.h'], subdir: milter_include_sub_dir)
headers += ['../server.h']

dependencies = [
  config,
  milter_core,
]
libmilter_server = library('milter-server',
                           c_args: '-DMILTER_LOG_DOMAIN="milter-server"',
                           sources: sources + headers + enums,
                           install: true,
                           dependencies: dependencies,
                           soversion: so_version,
                           version: library_version)
milter_server = declare_dependency(link_with: libmilter_server,
                                   dependencies: dependencies)

pkgconfig.generate(libmilter_server,
                   description: 'milter API',
                   filebase: 'milter-server',
                   name: 'milter server library',
                   requires: ['milter-core = ' + meson.project_version()],
                   variables: pkgconfig_variables)

dependencies = [
  declare_dependency(sources: milter_core_gir),
]
milter_server_gir = gnome.generate_gir(libmilter_server,
                                       dependencies: dependencies,
                                       export_packages: 'milter-server',
                                       extra_args: [
                                         '--warn-all',
                                       ],
                                       fatal_warnings: true,
                                       header: 'milter/server.h',
                                       identifier_prefix: 'Milter',
                                       includes: [
                                         'MilterCore-' + api_version,
                                         'GObject-2.0',
                                       ],
                                       install: true,
                                       namespace: 'MilterServer',
                                       nsversion: api_version,
                                       sources: sources + headers + enums,
                                       symbol_prefix: 'milter')
