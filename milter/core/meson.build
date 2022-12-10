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
version_components = meson.project_version().split('.')
version_h_conf.set('MILTER_MANAGER_VERSION', meson.project_version())
version_h_conf.set('MILTER_MANAGER_VERSION_MAJOR', version_components[0])
version_h_conf.set('MILTER_MANAGER_VERSION_MINOR', version_components[1])
version_h_conf.set('MILTER_MANAGER_VERSION_MICRO', version_components[2])
version_h = configure_file(input: 'milter-version.h.in',
                           output: 'milter-version.h',
                           configuration: version_h_conf)
headers += version_h

enums = gnome.mkenums_simple('milter-enum-types',
                             body_prefix: '#include <config.h>',
                             identifier_prefix: 'Milter',
                             install_dir: milter_include_dir / 'core',
                             install_header: true,
                             sources: headers,
                             symbol_prefix: 'milter')
enums_h = enums[1]

install_headers(headers, subdir: milter_include_sub_dir / 'core')
install_headers(['../core.h'], subdir: milter_include_sub_dir)
headers += ['../core.h']

gobject = dependency('gobject-2.0')
c_compiler = meson.get_compiler('c')
ev = declare_dependency(compile_args: '-DEV_COMPAT3=0',
                        dependencies: c_compiler.find_library('ev'))
dependencies = [
  config,
  gobject,
  ev,
]
libmilter_core = library('milter-core',
                         c_args: '-DMILTER_LOG_DOMAIN="milter-core"',
                         sources: sources + headers + enums,
                         install: true,
                         dependencies: dependencies,
                         soversion: so_version,
                         version: library_version)
milter_core = declare_dependency(dependencies: dependencies,
                                 link_with: libmilter_core,
                                 sources: [enums_h])

pkgconfig.generate(libmilter_core,
                   description: 'common milter features',
                   filebase: 'milter-core',
                   name: 'milter core library',
                   requires: ['gobject-2.0'],
                   subdirs: project_include_sub_dir,
                   variables: pkgconfig_variables)

milter_core_gir = gnome.generate_gir(libmilter_core,
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
                                     namespace: 'MilterCore',
                                     nsversion: api_version,
                                     sources: sources + headers + enums,
                                     symbol_prefix: 'milter')

if enable_doc
  gi_docgen_toml = configure_file(input: 'gi-docgen.toml.in',
                                  output: 'gi-docgen.toml',
                                  configuration: gi_docgen_toml_conf)
  custom_target('milter-core-doc',
                command: [
                  find_program('gi-docgen'),
                  'generate',
                  '--config=@INPUT0@',
                  '--output-dir=@OUTPUT0@',
                  '--no-namespace-dir',
                  '@INPUT1@',
                ],
                input: [
                  gi_docgen_toml,
                  milter_core_gir[0],
                ],
                install: true,
                install_dir: doc_dir,
                output: ['milter-core'])
endif
