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

class TestClientConfiguration
  class Base < Test::Unit::TestCase
    include MilterTestUtils

    def setup
      @client = ::Milter::Client.new
      @configuration = ::Milter::Client::Configuration.new
      @loader = ::Milter::Client::ConfigurationLoader.new(@configuration)
    end
  end

  class Milter < Base
    include MilterEventLoopTestUtils

    setup
    def setup_config
      @milter_config = @configuration.milter
    end

    setup
    def setup_loader
      @milter_loader = @loader.milter
    end

    def test_fallback_status
      assert_equal("accept", @milter_config.fallback_status)
      assert_equal("accept", @milter_loader.fallback_status)
      @milter_loader.fallback_status = "reject"
      assert_equal("reject", @milter_loader.fallback_status)
      assert_equal("reject", @milter_config.fallback_status)
    end

    def test_fallback_status_normalize
      @milter_loader.fallback_status = "Temporary_Failure"
      assert_equal("temporary-failure", @milter_loader.fallback_status)
    end

    def test_fallback_status_invalid
      invalid_value_class = ::Milter::Client::ConfigurationLoader::InvalidValue
      available_values = ["accept", "reject", "temporary-failure", "discard"]
      invalid_value = invalid_value_class.new("milter.fallback_status",
                                              available_values,
                                              "unknown")
      assert_raise(invalid_value) do
        @milter_loader.fallback_status = "unknown"
      end
    end

    def test_name
      @milter_config.name = "test-milter"
      assert_equal("test-milter", @milter_config.name)
      assert_equal("test-milter", @milter_loader.name)
    end

    def test_maintained
      n_called = {
        :before => 0,
        :after => 0,
      }
      @milter_loader.maintained do
        n_called[:before] += 1
        raise "failed"
        n_called[:after] += 1
      end
      assert_nothing_raised do
        @milter_config.maintained
      end
      assert_equal({:before => 1, :after => 0}, n_called)
    end

    def test_event_loop_created
      n_called = {
        :before => 0,
        :after => 0,
      }
      block_arguments = nil
      @milter_loader.event_loop_created do |*arguments|
        block_arguments = arguments
        n_called[:before] += 1
        raise "failed"
        n_called[:after] += 1
      end
      assert_nothing_raised do
        @milter_config.event_loop_created(create_event_loop)
      end
      assert_equal({:before => 1, :after => 0}, n_called)
      assert_equal([event_loop_class],
                   block_arguments.collect {|argument| argument.class})
    end
  end

  class Security < Base
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

  class Log < Base
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

  class Database < Base
    setup
    def setup_config
      @database_config = @configuration.database
    end

    setup
    def setup_loader
      @database_loader = @loader.database
    end

    def test_extra_options
      @database_loader.extra_options[:reconnect] = true
      assert_equal({
                     :type => nil,
                     :name => nil,
                     :host => nil,
                     :port => nil,
                     :path => nil,
                     :user => nil,
                     :password => nil,
                     :reconnect => true,
                   },
                   @database_config.to_hash)
    end
  end
end
