# Copyright (C) 2009-2013  Kouhei Sutou <kou@clear-code.com>
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

class TestLogger < Test::Unit::TestCase
  include MilterTestUtils

  def setup
    @logger = Milter::Logger.default
  end

  def test_log_level_constant
    assert_equal(Milter::LogLevelFlags::CRITICAL |
                 Milter::LogLevelFlags::ERROR |
                 Milter::LogLevelFlags::WARNING |
                 Milter::LogLevelFlags::MESSAGE |
                 Milter::LogLevelFlags::INFO |
                 Milter::LogLevelFlags::DEBUG |
                 Milter::LogLevelFlags::TRACE |
                 Milter::LogLevelFlags::STATISTICS |
                 Milter::LogLevelFlags::PROFILE,
                 Milter::LogLevelFlags::ALL)
    assert_equal(Milter::LOG_LEVEL_ALL,
                 Milter::LogLevelFlags::ALL)
  end

  def test_log_item_constant
    assert_equal(Milter::LogItemFlags::DOMAIN |
                 Milter::LogItemFlags::LEVEL |
                 Milter::LogItemFlags::LOCATION |
                 Milter::LogItemFlags::TIME |
                 Milter::LogItemFlags::NAME |
                 Milter::LogItemFlags::PID,
                 Milter::LogItemFlags::ALL)
    assert_equal(Milter::LOG_ITEM_ALL,
                 Milter::LogItemFlags::ALL)
  end

  def test_log_level_from_string
    assert_equal(Milter::LOG_LEVEL_ALL, Milter::LogLevelFlags.from_string("all"))
  end

  class Path < self
    def setup
      super
      @temporary_log_file = Tempfile.new("log")
      @logger.path = nil
    end

    def teardown
      super
      @logger.path = nil
      @temporary_log_file.close!
    end

    def test_nil
      @logger.path = nil
      assert_nil(@logger.path)
    end

    def test_existent
      @logger.path = @temporary_log_file.path
      assert_equal(@temporary_log_file.path, @logger.path)
    end

    def test_not_creatable
      assert_raise(GLib::FileError) do
        @logger.path = File.join(@temporary_log_file.path, "not-creatble")
      end
    end
  end

  class Reopen < self
    def setup
      super
      @temporary_log_file = Tempfile.new("log")
      @logger.path = @temporary_log_file.path
    end

    def teardown
      super
      @logger.path = nil
      @temporary_log_file.close!
    end

    def test_recreate
      path = @temporary_log_file.path
      FileUtils.rm_f(path)
      assert_false(File.exist?(path))
      @logger.reopen
      assert_true(File.exist?(path))
    end
  end
end
