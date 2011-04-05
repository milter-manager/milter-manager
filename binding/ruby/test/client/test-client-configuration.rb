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

class TestClientConfiguration < Test::Unit::TestCase
  include MilterTestUtils

  def setup
    @client = Milter::Client.new
    @configuration = Milter::Client::Configuration.new
    @loader = Milter::Client::ConfigurationLoader.new(@configuration)
  end

  class TestSecurity < TestClientConfiguration
    setup
    def setup_config
      @security_config = @configuration.security
    end

    setup
    def setup_loader
      @security_loader = @loader.security
    end

    def test_effective_user
      assert_nil(@security_config.effective_user)
      assert_nil(@security_loader.effective_user)
      @security_loader.effective_user = "milter"
      assert_equal("milter", @security_loader.effective_user)
      assert_equal("milter", @security_config.effective_user)
    end

    def test_effective_group
      assert_nil(@security_config.effective_group)
      assert_nil(@security_loader.effective_group)
      @security_loader.effective_group = "milter"
      assert_equal("milter", @security_loader.effective_group)
      assert_equal("milter", @security_config.effective_group)
    end
  end

  class TestLog < TestClientConfiguration
    setup
    def setup_config
      @log_config = @configuration.log
    end

    def test_use_syslog
      assert_not_predicate(@log_config, :use_syslog?)
      assert_not_predicate(@loader.log, :use_syslog?)
      @loader.log.use_syslog = true
      assert_predicate(@loader.log, :use_syslog?)
      assert_predicate(@log_config, :use_syslog?)
    end
  end
end
