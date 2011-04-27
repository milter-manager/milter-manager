# Copyright (C) 2011  Kouhei Sutou <kou@clear-code.com>
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

class TestApplicableConditionsTrust < Test::Unit::TestCase
  def setup
    @configuration = Milter::Manager::Configuration.new
    @configuration.clear_load_paths
    @configuration.append_load_path("#{ENV['TOP_SRCDIR']}/data")
    @loader = Milter::Manager::ConfigurationLoader.new(@configuration)
    @loader.load("applicable-conditions/trust.conf")
    @trust = @loader.trust
  end

  def test_load_envelope_from_domains
    @trust.clear
    domains = Tempfile.new("trust")
    domains.puts(<<-EOD)
example.com
EOD
    domains.close
    assert_not_send([@trust,
                     :trusted_envelope_from_domain?,
                     "<bob@example.com>"])
    @trust.load_envelope_from_domains(domains.path)
    assert_send([@trust,
                 :trusted_envelope_from_domain?,
                 "<bob@example.com>"])
  end
end
