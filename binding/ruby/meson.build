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

install_data('lib/milter.rb',
             install_dir: ruby_install_dir)
install_data('lib/milter/client.rb',
             'lib/milter/core.rb',
             'lib/milter/manager.rb',
             'lib/milter/server.rb',
             install_dir: ruby_install_dir / 'milter')
install_data('lib/milter/core/callback.rb',
             'lib/milter/core/command-runner.rb',
             'lib/milter/core/compatible.rb',
             'lib/milter/core/logger.rb',
             'lib/milter/core/macro.rb',
             'lib/milter/core/path.rb',
             'lib/milter/core/socket-address.rb',
             install_dir: ruby_install_dir / 'milter' / 'core')
install_data('lib/milter/client/command-line.rb',
             'lib/milter/client/composite-session.rb',
             'lib/milter/client/configuration.rb',
             'lib/milter/client/context-state.rb',
             'lib/milter/client/envelope-address.rb',
             'lib/milter/client/fallback-session.rb',
             'lib/milter/client/mail-transaction-shelf.rb',
             'lib/milter/client/session-context.rb',
             'lib/milter/client/session.rb',
             'lib/milter/client/testing.rb',
             install_dir: ruby_install_dir / 'milter' / 'client')
install_data('lib/milter/server/testing.rb',
             install_dir: ruby_install_dir / 'milter' / 'server')
install_data('lib/milter/manager/address-matcher.rb',
             'lib/milter/manager/breaker.rb',
             'lib/milter/manager/child-context.rb',
             'lib/milter/manager/clamav-milter-config-parser.rb',
             'lib/milter/manager/condition-table.rb',
             'lib/milter/manager/connection-check-context.rb',
             'lib/milter/manager/debian-detector.rb',
             'lib/milter/manager/debian-init-detector.rb',
             'lib/milter/manager/debian-systemd-detector.rb',
             'lib/milter/manager/detector.rb',
             'lib/milter/manager/enma-socket-detector.rb',
             'lib/milter/manager/exception.rb',
             'lib/milter/manager/file-reader.rb',
             'lib/milter/manager/freebsd-rc-detector.rb',
             'lib/milter/manager/init-detector.rb',
             'lib/milter/manager/milter-greylist-config-parser.rb',
             'lib/milter/manager/netstat-connection-checker.rb',
             'lib/milter/manager/opendkim-config-parser.rb',
             'lib/milter/manager/pkgsrc-rc-detector.rb',
             'lib/milter/manager/policy-manager.rb',
             'lib/milter/manager/postfix-cidr-table.rb',
             'lib/milter/manager/postfix-condition-table-parser.rb',
             'lib/milter/manager/postfix-regexp-table.rb',
             'lib/milter/manager/rcng-detector.rb',
             'lib/milter/manager/redhat-detector.rb',
             'lib/milter/manager/redhat-init-detector.rb',
             'lib/milter/manager/redhat-systemd-detector.rb',
             'lib/milter/manager/redhat-upstart-detector.rb',
             'lib/milter/manager/rmilter-socket-detector.rb',
             'lib/milter/manager/rspamd-proxy-detector.rb',
             'lib/milter/manager/systemd-detector.rb',
             install_dir: ruby_install_dir / 'milter' / 'manager')

test_env = environment()
test_run_test_rb = files('test/run-test.rb')
test('ruby',
     ruby,
     args: [test_run_test_rb],
     env: test_env)
