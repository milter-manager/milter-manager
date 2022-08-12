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
  'milter-client-context.c',
  'milter-client-main.c',
  'milter-client-runner.c',
  'milter-client-single-thread-runner.c',
  'milter-client.c',
)

headers = files(
  'milter-client-context.h',
  'milter-client-objects.h',
  'milter-client-runner.h',
  'milter-client-single-thread-runner.h',
  'milter-client.h',
)

enums = gnome.mkenums_simple('milter-client-enum-types',
                             body_prefix: '#include <config.h>',
                             identifier_prefix: 'Milter',
                             install_dir: milter_include_dir / 'client',
                             install_header: true,
                             sources: headers,
                             symbol_prefix: 'milter')

install_headers(headers, subdir: milter_include_sub_dir / 'client')
install_headers(['../client.h'], subdir: milter_include_sub_dir)
headers += ['../client.h']

dependencies = [
  config,
  milter_core,
]
libmilter_client = library('milter-client',
                           c_args: '-DMILTER_LOG_DOMAIN="milter-client"',
                           sources: sources + headers + enums,
                           install: true,
                           dependencies: dependencies,
                           soversion: so_version,
                           version: library_version)
milter_client = declare_dependency(link_with: libmilter_client,
                                   dependencies: dependencies)

pkgconfig.generate(libmilter_client,
                   description: 'milter API',
                   filebase: 'milter-client',
                   name: 'milter client library',
                   requires: ['milter-core = ' + meson.project_version()],
                   variables: pkgconfig_variables)

dependencies = [
  declare_dependency(sources: milter_core_gir),
]
milter_client_gir = gnome.generate_gir(libmilter_client,
                                       dependencies: dependencies,
                                       export_packages: 'milter-client',
                                       extra_args: [
                                         '--warn-all',
                                       ],
                                       fatal_warnings: true,
                                       header: 'milter/client.h',
                                       identifier_prefix: 'Milter',
                                       includes: [
                                         'MilterCore-' + api_version,
                                         'GObject-2.0',
                                       ],
                                       install: true,
                                       namespace: 'MilterClient',
                                       nsversion: api_version,
                                       sources: sources + headers + enums,
                                       symbol_prefix: 'milter')
