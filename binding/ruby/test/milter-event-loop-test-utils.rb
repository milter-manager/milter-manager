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

module MilterEventLoopTestUtils
  include MilterTestUtils

  private
  def event_loop_class
    backend = ENV["MILTER_EVENT_LOOP_BACKEND"] || "glib"
    case backend
    when "glib"
      Milter::GLibEventLoop
    when "libev"
      Milter::LibevEventLoop
    else
      raise "unknown backend: #{backend.inspect}"
    end
  end

  def create_event_loop
    backend = ENV["MILTER_EVENT_LOOP_BACKEND"] || "glib"
    case backend
    when "glib"
      Milter::GLibEventLoop.new
    when "libev"
      Milter::LibevEventLoop.default
    else
      raise "unknown backend: #{backend.inspect}"
    end
  end
end
