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
    class Error < StandardError
    end

    class EventLoop
      alias_method :add_idle_raw, :add_idle
      def add_idle(priority: nil, &block)
        priority ||= GLib::PRIORITY_DEFAULT_IDLE
        add_idle_raw(priority, &block)
      end
    end

    LogLevelFlags.values.each do |value|
      const_set("LOG_LEVEL_#{value.nick.upcase}", value)
    end

    class StepFlags
      NO_EVENT_MASK = new(Milter::STEP_NO_EVENT_MASK)
      NO_REPLY_MASK = new(Milter::STEP_NO_REPLY_MASK)
      NO_MASK = (NO_EVENT_MASK | NO_REPLY_MASK)
      YES_MASK = new(Milter::STEP_YES_MASK)
    end
  end
end

require 'milter/core/compatible'

require 'milter/core/logger'
require 'milter/core/socket-address'
require 'milter/core/macro'
require 'milter/core/path'
require 'milter/core/callback'
