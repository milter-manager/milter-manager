# Copyright (C) 2008-2022  Sutou Kouhei <kou@clear-code.com>
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

require 'glib2'
GLib::Log.cancel_handler

begin
  require "milter_core.so"
rescue LoadError
  require "gobject-introspection"
  Milter = GObjectIntrospection.load("MilterCore")
  Milter.init
  module Milter
    class << self
      def define_backward_compatible_enum_constants(target, enum, prefix)
        enum.values.each do |value|
          name = "#{prefix}_" + value.nick.gsub(/-/, "_").upcase
          target.const_set(name, value)
        end
      end
    end

    VERSION = [
      TOOLKIT_VERSION_MAJOR,
      TOOLKIT_VERSION_MINOR,
      TOOLKIT_VERSION_MICRO,
    ].freeze

    class Error < StandardError
    end

    class CommandEncoder
      alias_method :encode_connect_raw, :encode_connect
      def encode_connect(host_name, address)
        encode_connect_raw(host_name, address.pack)
      end
    end

    class EventLoop
      alias_method :add_idle_raw, :add_idle
      def add_idle(priority: nil, &block)
        priority ||= GLib::PRIORITY_DEFAULT_IDLE
        add_idle_raw(priority, &block)
      end

      alias_method :add_timeout_raw, :add_timeout
      def add_timeout(interval_in_seconds, priority: nil, &block)
        priority ||= GLib::PRIORITY_DEFAULT_IDLE
        add_timeout_raw(priority, interval_in_seconds, &block)
      end

      alias_method :watch_io_raw, :watch_io
      def watch_io(channel, condition, priority: nil, &block)
        priority ||= GLib::PRIORITY_DEFAULT_IDLE
        watch_io_raw(priority, channel, condition, &block)
      end
    end

    class Logger
      alias_method :set_target_level_raw, :set_target_level
      def set_target_level(level)
        case level
        when Numeric, LogLevelFlags
          set_target_level_raw(level)
        else
          set_target_level_by_string(level)
        end
      end
      alias_method :target_level_raw=, :target_level=
      alias_method :target_level=, :set_target_level
    end

    define_backward_compatible_enum_constants(self, ActionFlags, "ACTION")
    define_backward_compatible_enum_constants(self, LogLevelFlags, "LOG_LEVEL")
    define_backward_compatible_enum_constants(self, Status, "STATUS")

    class StepFlags
      NO_EVENT_MASK = new(Milter::STEP_NO_EVENT_MASK)
      NO_REPLY_MASK = new(Milter::STEP_NO_REPLY_MASK)
      NO_MASK = (NO_EVENT_MASK | NO_REPLY_MASK)
      YES_MASK = new(Milter::STEP_YES_MASK)
    end
  end
end

require "milter/core/compatible"

require "milter/core/callback"
require "milter/core/command-runner"
require "milter/core/logger"
require "milter/core/macro"
require "milter/core/path"
require "milter/core/socket-address"
