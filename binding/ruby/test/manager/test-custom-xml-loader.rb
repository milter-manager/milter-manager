# Copyright (C) 2008  Kouhei Sutou <kou@cozmixng.org>
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

class TestCustomXMLLoader < Test::Unit::TestCase
  include MilterTestUtils

  def setup
    @configuration = Milter::Manager::Configuration.new
    @loader = Milter::Manager::ConfigurationLoader.new(@configuration)
  end

  def test_security
    assert_false(@configuration.privilege_mode?)
    load(<<-EOC)
<configuration>
  <security>
    <privilege-mode>true</privilege-mode>
  </security>
</configuration>
EOC
    assert_true(@configuration.privilege_mode?)
  end

  def test_milter
    @loader.define_applicable_condition("S25R") do |condition|
      condition.description = "Selective SMTP Rejection"
    end

    load(<<-EOC)
<configuration>
  <milters>
    <milter>
      <name>milter-greylist</name>
      <enabled>false</enabled>
      <connection-spec>inet:10026@localhost</connection-spec>
      <command>/etc/init.d/milter-greylist</command>
      <command-options>start</command-options>
      <applicable-conditions>
        <applicable-condition>S25R</applicable-condition>
      </applicable-conditions>
    </milter>
  </milters>
</configuration>
EOC

    assert_equal(<<-EOX, @configuration.to_xml)
<configuration>
  <applicable-conditions>
    <applicable-condition>
      <name>S25R</name>
      <description>Selective SMTP Rejection</description>
    </applicable-condition>
  </applicable-conditions>
  <milters>
    <milter>
      <name>milter-greylist</name>
      <enabled>false</enabled>
      <fallback-status>accept</fallback-status>
      <evaluation-mode>false</evaluation-mode>
      <connection-spec>inet:10026@localhost</connection-spec>
      <command>/etc/init.d/milter-greylist</command>
      <command-options>start</command-options>
      <applicable-conditions>
        <applicable-condition>S25R</applicable-condition>
      </applicable-conditions>
    </milter>
  </milters>
</configuration>
EOX
  end

  def test_milter_fallback_status
    load(<<-EOC)
<configuration>
  <milters>
    <milter>
      <name>milter-greylist</name>
      <enabled>false</enabled>
      <fallback-status>reject</fallback-status>
      <connection-spec>inet:10026@localhost</connection-spec>
      <command>/etc/init.d/milter-greylist</command>
      <command-options>start</command-options>
    </milter>
  </milters>
</configuration>
EOC

    assert_equal(<<-EOX, @configuration.to_xml)
<configuration>
  <milters>
    <milter>
      <name>milter-greylist</name>
      <enabled>false</enabled>
      <fallback-status>reject</fallback-status>
      <evaluation-mode>false</evaluation-mode>
      <connection-spec>inet:10026@localhost</connection-spec>
      <command>/etc/init.d/milter-greylist</command>
      <command-options>start</command-options>
    </milter>
  </milters>
</configuration>
EOX
  end

  def test_change_applicable_conditions
    @loader.define_applicable_condition("S25R") do |condition|
      condition.description = "Selective SMTP Rejection"
    end
    @loader.define_applicable_condition("Remote Network") do |condition|
      condition.description = "Remote Network"
    end

    @loader.define_milter("milter-greylist") do |milter|
      milter.connection_spec = "inet:10026"
      milter.applicable_conditions = ["Remote Network"]
    end

    load(<<-EOC)
<configuration>
  <milters>
    <milter>
      <name>milter-greylist</name>
      <applicable-conditions>
        <applicable-condition>S25R</applicable-condition>
      </applicable-conditions>
    </milter>
  </milters>
</configuration>
EOC

    assert_equal(<<-EOX, @configuration.to_xml)
<configuration>
  <applicable-conditions>
    <applicable-condition>
      <name>S25R</name>
      <description>Selective SMTP Rejection</description>
    </applicable-condition>
    <applicable-condition>
      <name>Remote Network</name>
      <description>Remote Network</description>
    </applicable-condition>
  </applicable-conditions>
  <milters>
    <milter>
      <name>milter-greylist</name>
      <enabled>true</enabled>
      <fallback-status>accept</fallback-status>
      <evaluation-mode>false</evaluation-mode>
      <connection-spec>inet:10026</connection-spec>
      <applicable-conditions>
        <applicable-condition>S25R</applicable-condition>
      </applicable-conditions>
    </milter>
  </milters>
</configuration>
EOX
  end

  def test_remove_applicable_conditions
    @loader.define_applicable_condition("S25R") do |condition|
      condition.description = "Selective SMTP Rejection"
    end
    @loader.define_applicable_condition("Remote Network") do |condition|
      condition.description = "Remote Network"
    end

    @loader.define_milter("milter-greylist") do |milter|
      milter.connection_spec = "inet:10026"
      milter.applicable_conditions = ["S25R", "Remote Network"]
    end

    load(<<-EOC)
<configuration>
  <milters>
    <milter>
      <name>milter-greylist</name>
    </milter>
  </milters>
</configuration>
EOC

    assert_equal(<<-EOX, @configuration.to_xml)
<configuration>
  <applicable-conditions>
    <applicable-condition>
      <name>S25R</name>
      <description>Selective SMTP Rejection</description>
    </applicable-condition>
    <applicable-condition>
      <name>Remote Network</name>
      <description>Remote Network</description>
    </applicable-condition>
  </applicable-conditions>
  <milters>
    <milter>
      <name>milter-greylist</name>
      <enabled>true</enabled>
      <fallback-status>accept</fallback-status>
      <evaluation-mode>false</evaluation-mode>
      <connection-spec>inet:10026</connection-spec>
    </milter>
  </milters>
</configuration>
EOX
  end

  private
  def load(content)
    file = Tempfile.new("xml-loader")
    file.open
    file.puts(content)
    file.close
    @loader.load_custom_configuration(file.path)
  end
end
