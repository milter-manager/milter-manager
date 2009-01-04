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
      <name>milter@10026</name>
      <enabled>false</enabled>
      <connection-spec>inet:10026@localhost</connection-spec>
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
      <name>milter@10026</name>
      <enabled>false</enabled>
      <connection-spec>inet:10026@localhost</connection-spec>
      <applicable-conditions>
        <applicable-condition>S25R</applicable-condition>
      </applicable-conditions>
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
