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
  'milter-manager-applicable-condition.c',
  'milter-manager-child.c',
  'milter-manager-children.c',
  'milter-manager-configuration.c',
  'milter-manager-control-command-decoder.c',
  'milter-manager-control-command-encoder.c',
  'milter-manager-control-reply-decoder.c',
  'milter-manager-control-reply-encoder.c',
  'milter-manager-controller-context.c',
  'milter-manager-controller.c',
  'milter-manager-egg.c',
  'milter-manager-launch-command-decoder.c',
  'milter-manager-launch-command-encoder.c',
  'milter-manager-leader.c',
  'milter-manager-main.c',
  'milter-manager-module.c',
  'milter-manager-process-launcher.c',
  'milter-manager-reply-decoder.c',
  'milter-manager-reply-encoder.c',
  'milter-manager.c',
)

headers = files(
  'milter-manager-applicable-condition.h',
  'milter-manager-child.h',
  'milter-manager-children.h',
  'milter-manager-configuration.h',
  'milter-manager-control-command-decoder.h',
  'milter-manager-control-command-encoder.h',
  'milter-manager-control-protocol.h',
  'milter-manager-control-reply-decoder.h',
  'milter-manager-control-reply-encoder.h',
  'milter-manager-controller-context.h',
  'milter-manager-controller.h',
  'milter-manager-egg.h',
  'milter-manager-launch-command-decoder.h',
  'milter-manager-launch-command-encoder.h',
  'milter-manager-launch-protocol.h',
  'milter-manager-leader.h',
  'milter-manager-module-impl.h',
  'milter-manager-module.h',
  'milter-manager-objects.h',
  'milter-manager-process-launcher.h',
  'milter-manager-reply-decoder.h',
  'milter-manager-reply-encoder.h',
  'milter-manager-reply-protocol.h',
  'milter-manager.h',
)

enums = gnome.mkenums_simple('milter-manager-enum-types',
                             body_prefix: '#include <config.h>',
                             identifier_prefix: 'Milter',
                             install_dir: milter_include_dir / 'manager',
                             install_header: true,
                             sources: headers,
                             symbol_prefix: 'milter')

install_headers(headers, subdir: milter_include_sub_dir / 'manager')
install_headers(['../manager.h'], subdir: milter_include_sub_dir)
headers += ['../manager.h']

dependencies = [
  config,
  milter_client,
  milter_server,
  dependency('gmodule-2.0'),
]
libmilter_manager = library('milter-manager',
                           c_args: '-DMILTER_LOG_DOMAIN="milter-manager"',
                           sources: sources + headers + enums,
                           install: true,
                           dependencies: dependencies,
                           soversion: so_version,
                           version: library_version)

pkgconfig.generate(libmilter_manager,
                   description: 'milter API',
                   filebase: 'milter-manager',
                   name: 'milter manager library',
                   requires: [
                     'milter-client = ' + meson.project_version(),
                     'milter-server = ' + meson.project_version(),
                     'gmodule-2.0',
                   ],
                   variables: pkgconfig_variables)

dependencies = [
  declare_dependency(sources: milter_core_gir),
  declare_dependency(sources: milter_client_gir),
  declare_dependency(sources: milter_server_gir),
]
gnome.generate_gir(libmilter_manager,
                   dependencies: dependencies,
                   export_packages: 'milter-manager',
                   extra_args: [
                     '--warn-all',
                   ],
                   fatal_warnings: true,
                   header: 'milter/manager.h',
                   identifier_prefix: 'Milter',
                   includes: [
                     'MilterClient-' + api_version,
                     'MilterServer-' + api_version,
                     'GObject-2.0',
                   ],
                   install: true,
                   namespace: 'MilterManager',
                   nsversion: api_version,
                   sources: sources + headers + enums,
                   symbol_prefix: 'milter')
