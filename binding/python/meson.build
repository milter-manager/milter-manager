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

python.install_sources(
    'gi/overrides/MilterClient/__init__.py',
    'gi/overrides/MilterClient/client.py',
    'gi/overrides/MilterClient/client_context.py',
    'gi/overrides/MilterClient/command_line.py',
    'gi/overrides/MilterClient/milter_runner.py',
    'gi/overrides/MilterClient/session.py',
    'gi/overrides/MilterClient/session_context.py',
    subdir: 'gi/overrides/MilterClient/',
)
python.install_sources(
    'gi/overrides/MilterCore/__init__.py',
    'gi/overrides/MilterCore/agent.py',
    'gi/overrides/MilterCore/logger.py',
    subdir: 'gi/overrides/MilterCore/',
)
python.install_sources(
    'gi/overrides/MilterServer/__init__.py',
    'gi/overrides/MilterServer/test_server.py',
    subdir: 'gi/overrides/MilterServer/',
)
python.install_sources('milter/__init__.py', subdir: 'milter/')
python.install_sources('milter/client/__init__.py', subdir: 'milter/client/')
python.install_sources('milter/core/__init__.py', subdir: 'milter/core/')
python.install_sources('milter/server/__init__.py', subdir: 'milter/server/')

python_sample_dir = data_dir / meson.project_name() / 'python' / 'sample'
install_data(
    'sample/milter-external.py',
    'sample/milter-replace.py',
    install_dir: python_sample_dir,
)
