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
  'milter-agent.c',
  'milter-command-decoder.c',
  'milter-command-encoder.c',
  'milter-connection.c',
  'milter-core-internal.h',
  'milter-core.c',
  'milter-decoder.c',
  'milter-encoder.c',
  'milter-error-emittable.c',
  'milter-esmtp.c',
  'milter-event-loop.c',
  'milter-finished-emittable.c',
  'milter-glib-event-loop.c',
  'milter-headers.c',
  'milter-libev-event-loop.c',
  'milter-logger.c',
  'milter-macros-requests.c',
  'milter-message-result.c',
  'milter-option.c',
  'milter-protocol-agent.c',
  'milter-protocol.c',
  'milter-reader.c',
  'milter-reply-decoder.c',
  'milter-reply-encoder.c',
  'milter-reply-signals.c',
  'milter-session-result.c',
  'milter-syslog-logger.c',
  'milter-utils.c',
  'milter-writer.c',
)

headers = files(
  'milter-agent.h',
  'milter-command-decoder.h',
  'milter-command-encoder.h',
  'milter-connection.h',
  'milter-decoder.h',
  'milter-encoder.h',
  'milter-error-emittable.h',
  'milter-esmtp.h',
  'milter-event-loop.h',
  'milter-finished-emittable.h',
  'milter-glib-event-loop.h',
  'milter-headers.h',
  'milter-libev-event-loop.h',
  'milter-logger.h',
  'milter-macros-requests.h',
  'milter-message-result.h',
  'milter-option.h',
  'milter-protocol-agent.h',
  'milter-protocol.h',
  'milter-reader.h',
  'milter-reply-decoder.h',
  'milter-reply-encoder.h',
  'milter-reply-signals.h',
  'milter-session-result.h',
  'milter-syslog-logger.h',
  'milter-utils.h',
  'milter-writer.h',
)

version_h_conf = configuration_data()
version_components = meson.version().split('.')
version_h_conf.set('MILTER_MANAGER_VERSION', meson.version())
version_h_conf.set('MILTER_MANAGER_VERSION_MAJOR', version_components[0])
version_h_conf.set('MILTER_MANAGER_VERSION_MINOR', version_components[1])
version_h_conf.set('MILTER_MANAGER_VERSION_MICRO', version_components[2])
version_h = configure_file(input: 'milter-version.h.in',
                           output: 'milter-version.h',
                           configuration: version_h_conf)
headers += version_h

# enums = gnome.mkenums('milter-enum-types',
#                       sources: headers,
#                       identifier_prefix: 'Milter',
#                       symbol_prefix: 'milter',
#                       c_template: 'enums.c.template',
#                       h_template: 'enums.h.template',
#                       install_dir: join_paths(include_dir, 'milter', 'core'),
#                       install_header: true)
enums = gnome.mkenums_simple('milter-enum-types', sources: headers)
enums_source = enums[0]
enums_header = enums[1]

install_headers(headers, subdir: join_paths('milter', 'core'))

gobject = dependency('gobject-2.0')
gthread = dependency('gthread-2.0')
dependencies = [
  gobject,
  gthread,
]
milter_core = library('milter-core',
                      sources: sources + enums,
                      install: true,
                      dependencies: dependencies,
                      include_directories: base_include_directories,
                      soversion: so_version,
                      version: library_version)

pkgconfig.generate(milter_core,
                   description: 'common milter features',
                   filebase: 'milter-core',
                   name: 'milter core library',
                   requires: ['gobject-2.0', 'gthread-2.0'],
                   variables: pkgconfig_variables,
                   version: meson.version())

milter_core_gir = gnome.generate_gir(milter_core,
                                     export_packages: 'milter-core',
                                     extra_args: [
                                       '--warn-all',
                                     ],
                                     fatal_warnings: true,
                                     header: 'milter/core.h',
                                     identifier_prefix: 'Milter',
                                     includes: [
                                       'GObject-2.0',
                                     ],
                                     install: true,
                                     namespace: 'Milter',
                                     nsversion: api_version,
                                     sources: sources + headers + enums,
                                     symbol_prefix: 'milter')
