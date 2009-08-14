# Copyright (C) 2008-2009  Kouhei Sutou <kou@clear-code.com>
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

class TestConfigurationLoader < Test::Unit::TestCase
  def setup
    @configuration = Milter::Manager::Configuration.new
    @loader = Milter::Manager::ConfigurationLoader.new(@configuration)
  end

  def test_package_platform
    @loader.package.platform = "new-platform"
    assert_equal("new-platform", @configuration.package_platform)
    assert_equal("new-platform", @loader.package.platform)
  end

  def test_package_options
    @loader.package.options = "prefix=/usr/pkg"
    assert_equal("prefix=/usr/pkg", @configuration.package_options)
    assert_equal({"prefix" => "/usr/pkg"}, @loader.package.options)
  end

  def test_security_privilege_mode
    assert_false(@configuration.privilege_mode?)
    @loader.security.privilege_mode = true
    assert_true(@configuration.privilege_mode?)
  end

  def test_security_effective_user
    assert_nil(@configuration.effective_user)
    @loader.security.effective_user = "nobody"
    assert_equal("nobody", @configuration.effective_user)
  end

  def test_security_effective_group
    assert_nil(@configuration.effective_group)
    @loader.security.effective_group = "nogroup"
    assert_equal("nogroup", @configuration.effective_group)
  end

  def test_manager_unix_socket_mode
    assert_equal(0660, @configuration.manager_unix_socket_mode)
    @loader.manager.unix_socket_mode = 0600
    assert_equal(0600, @configuration.manager_unix_socket_mode)
  end

  def test_manager_unix_socket_group
    assert_nil(@configuration.manager_unix_socket_group)
    @loader.manager.unix_socket_group = "nogroup"
    assert_equal("nogroup", @configuration.manager_unix_socket_group)
  end

  def test_manager_remove_unix_socket_on_close
    assert_true(@configuration.remove_manager_unix_socket_on_close?)
    @loader.manager.remove_unix_socket_on_close = false
    assert_false(@configuration.remove_manager_unix_socket_on_close?)
  end

  def test_manager_remove_unix_socket_on_create
    assert_true(@configuration.remove_manager_unix_socket_on_create?)
    @loader.manager.remove_unix_socket_on_create = false
    assert_false(@configuration.remove_manager_unix_socket_on_create?)
  end

  def test_controller_unix_socket_mode
    assert_equal(0660, @configuration.controller_unix_socket_mode)
    @loader.controller.unix_socket_mode = 0600
    assert_equal(0600, @configuration.controller_unix_socket_mode)
  end

  def test_controller_unix_socket_group
    assert_nil(@configuration.controller_unix_socket_group)
    @loader.controller.unix_socket_group = "nogroup"
    assert_equal("nogroup", @configuration.controller_unix_socket_group)
  end

  def test_controller_remove_unix_socket_on_close
    assert_true(@configuration.remove_controller_unix_socket_on_close?)
    @loader.controller.remove_unix_socket_on_close = false
    assert_false(@configuration.remove_controller_unix_socket_on_close?)
  end

  def test_controller_remove_unix_socket_on_create
    assert_true(@configuration.remove_controller_unix_socket_on_create?)
    @loader.controller.remove_unix_socket_on_create = false
    assert_false(@configuration.remove_controller_unix_socket_on_create?)
  end

  def test_controller_connection_spec
    assert_nil(@configuration.controller_connection_spec)
    @loader.controller.connection_spec = "inet:2929@localhost"
    assert_equal("inet:2929@localhost",
                 @configuration.controller_connection_spec)
  end

  def test_manager_daemon
    assert_false(@configuration.daemon?)
    @loader.manager.daemon = true
    assert_true(@configuration.daemon?)
    assert_equal(@configuration.daemon?, @loader.manager.daemon?)
  end

  def test_manager_pid_file
    assert_nil(@configuration.pid_file)
    @loader.manager.pid_file = "/var/run/milter-manager.pid"
    assert_equal("/var/run/milter-manager.pid", @configuration.pid_file)
    assert_equal(@configuration.pid_file, @loader.manager.pid_file)
  end

  def test_manager_maintenance_interval
    assert_equal(100, @configuration.maintenance_interval)

    @loader.manager.maintenance_interval = nil
    assert_equal(0, @configuration.maintenance_interval)

    @loader.manager.maintenance_interval = 0
    assert_equal(0, @configuration.maintenance_interval)

    @loader.manager.maintenance_interval = 10
    assert_equal(10, @configuration.maintenance_interval)
  end

  def test_manager_suspend_time_on_unacceptable
    assert_equal(5, @configuration.suspend_time_on_unacceptable)

    @loader.manager.suspend_time_on_unacceptable = 0
    assert_equal(0, @configuration.suspend_time_on_unacceptable)

    @loader.manager.suspend_time_on_unacceptable = nil
    assert_equal(5, @configuration.suspend_time_on_unacceptable)

    @loader.manager.suspend_time_on_unacceptable = 10
    assert_equal(10, @configuration.suspend_time_on_unacceptable)
  end

  def test_manager_max_connections
    assert_equal(0, @configuration.max_connections)

    @loader.manager.max_connections = 5
    assert_equal(5, @configuration.max_connections)

    @loader.manager.max_connections = nil
    assert_equal(0, @configuration.max_connections)

    @loader.manager.max_connections = 10
    assert_equal(10, @configuration.max_connections)
  end

  def test_manager_configuration_directory
    assert_nil(@configuration.custom_configuration_directory)
    @loader.manager.custom_configuration_directory = "/tmp/milter-manager/"
    assert_equal("/tmp/milter-manager/",
                 @configuration.custom_configuration_directory)
    assert_equal(@configuration.custom_configuration_directory,
                 @loader.manager.custom_configuration_directory)
  end

  def test_to_xml
    assert_equal(<<-EOX, @configuration.to_xml)
<configuration>
</configuration>
EOX

    @loader.define_applicable_condition("remote-network") do |condition|
      condition.description = "Check only remote network"

      condition.define_connect_stopper do |context, host, address|
        host == "localhost"
      end

      condition.define_envelope_from_stopper do |context, from|
        from =~ /@my-domain1.example.com\z/
      end

      condition.define_envelope_recipient_stopper do |context, recipient|
        recipient =~ /@my-domain2.example.com\z/
      end

      condition.define_header_stopper do |context, name, value|
        name == "X-Internal" and value =~ /\A(yes|true)\z/i
      end
    end

    @loader.define_milter("child-milter") do |milter|
      milter.connection_spec = "unix:/tmp/socket"
      milter.add_applicable_condition("remote-network")
    end

    assert_equal(<<-EOX, @configuration.to_xml)
<configuration>
  <applicable-conditions>
    <applicable-condition>
      <name>remote-network</name>
      <description>Check only remote network</description>
    </applicable-condition>
  </applicable-conditions>
  <milters>
    <milter>
      <name>child-milter</name>
      <enabled>true</enabled>
      <fallback-status>accept</fallback-status>
      <evaluation-mode>false</evaluation-mode>
      <connection-spec>unix:/tmp/socket</connection-spec>
      <applicable-conditions>
        <applicable-condition>remote-network</applicable-condition>
      </applicable-conditions>
    </milter>
  </milters>
</configuration>
EOX
  end


  def test_applicable_conditions
    @loader.define_applicable_condition("S25R") do |condition|
      condition.description = "Selective SMTP Rejection."
      condition.define_connect_stopper do |child, host, address|
        case host
        when "unknown",
          /\A[^.]*\d[^\d.]+\d/,
          /\A[^.]*\d{5}/,
          /\A(?:[^.]+\.)?\d[^.]*\.[^.]+\..+\.[a-z]/i,
          /\A[^.]*\d\.[^.]*\d-\d/,
          /\A[^.]*\d\.[^.]*\d\.[^.]+\..+\./,
          /\A(?:dhcp|dialup|ppp|[achrsvx]?dsl)[^.]*\d/i
          false
        else
          true
        end
      end
    end

    assert_equal(["<configuration>",
                  "  <applicable-conditions>",
                  "    <applicable-condition>",
                  "      <name>S25R</name>",
                  "      <description>Selective SMTP Rejection.</description>",
                  "    </applicable-condition>",
                  "  </applicable-conditions>",
                  "</configuration>"].join("\n") + "\n",
                 @configuration.to_xml)
  end

  def test_define_milter_again
    @loader.define_milter("milter1") do |milter|
      milter.description = "description"
      milter.enabled = true
    end
    assert_equal([["milter1", true, "description"]],
                 @configuration.eggs.collect do |egg|
                   [egg.name, egg.enabled?, egg.description]
                 end)

    @loader.define_milter("milter1") do |milter|
    end
    assert_equal([["milter1", true, "description"]],
                 @configuration.eggs.collect do |egg|
                   [egg.name, egg.enabled?, egg.description]
                 end)

    @loader.define_milter("milter1") do |milter|
      milter.description = "new description"
      milter.enabled = false
    end
    assert_equal([["milter1", false, "new description"]],
                 @configuration.eggs.collect do |egg|
                   [egg.name, egg.enabled?, egg.description]
                 end)
  end

  def test_defined_milters
    @loader.define_milter("milter1") do |milter|
      milter.description = "description1"
      milter.enabled = true
    end
    @loader.define_milter("milter2") do |milter|
      milter.description = "description2"
    end

    @loader.defined_milters.each do |name|
      @loader.define_milter(name) do |milter|
        milter.description = "rewrite - #{milter.description}"
      end
    end

    assert_equal([["milter1", true, "rewrite - description1"],
                  ["milter2", true, "rewrite - description2"]],
                 @configuration.eggs.collect do |egg|
                   [egg.name, egg.enabled?, egg.description]
                 end)
  end

  def test_define_milter_set_applicable_conditions
    @loader.define_applicable_condition("S25R") do |condition|
      condition.description = "Selective SMTP Rejection."
    end
    @loader.define_applicable_condition("Remote Network") do |condition|
      condition.description = "Only works for remote network."
    end

    @loader.define_milter("milter1") do |milter|
      milter.applicable_conditions = ["S25R", "Remote Network"]
    end
    assert_equal([["milter1", ["S25R", "Remote Network"]]],
                 @configuration.eggs.collect do |egg|
                   [egg.name,
                    egg.applicable_conditions.collect {|cond| cond.name}]
                 end)

    @loader.define_milter("milter2") do |milter|
      milter.applicable_conditions = ["S25R", "Remote Network"]
      milter.applicable_conditions = ["S25R"]
    end
    assert_equal([["milter1", ["S25R", "Remote Network"]],
                  ["milter2", ["S25R"]]],
                 @configuration.eggs.collect do |egg|
                   [egg.name,
                    egg.applicable_conditions.collect {|cond| cond.name}]
                 end)
  end

  def test_define_milter_add_applicable_condition
    @loader.define_applicable_condition("S25R") do |condition|
      condition.description = "Selective SMTP Rejection."
    end
    @loader.define_applicable_condition("Remote Network") do |condition|
      condition.description = "Only works for remote network."
    end

    @loader.define_milter("milter1") do |milter|
      milter.applicable_conditions = ["S25R"]
      milter.applicable_conditions += ["Remote Network"]
    end
    assert_equal([["milter1", ["S25R", "Remote Network"]]],
                 @configuration.eggs.collect do |egg|
                   [egg.name,
                    egg.applicable_conditions.collect {|cond| cond.name}]
                 end)
  end

  def test_define_milter_fallback_status
    @loader.define_milter("milter") do |milter|
      assert_equal("accept", milter.fallback_status)
      milter.fallback_status = "temporary_failure"
      assert_equal("temporary-failure", milter.fallback_status)
    end
  end

  def test_define_milter_invalid_fallback_status
    @loader.define_milter("milter") do |milter|
      assert_raise(Milter::Manager::ConfigurationLoader::InvalidValue) do
        milter.fallback_status = "default"
      end
    end
  end

  def test_define_applicable_condition_again
    @loader.define_applicable_condition("S25R") do |condition|
      condition.description = "Selective SMTP Rejection."
    end
    assert_equal([["S25R", "Selective SMTP Rejection."]],
                 @configuration.applicable_conditions.collect do |condition|
                   [condition.name, condition.description]
                 end)

    @loader.define_applicable_condition("S25R") do |condition|
    end
    assert_equal([["S25R", "Selective SMTP Rejection."]],
                 @configuration.applicable_conditions.collect do |condition|
                   [condition.name, condition.description]
                 end)

    @loader.define_applicable_condition("S25R") do |condition|
      condition.description += " (useful)"
    end
    assert_equal([["S25R", "Selective SMTP Rejection. (useful)"]],
                 @configuration.applicable_conditions.collect do |condition|
                   [condition.name, condition.description]
                 end)
  end
end
