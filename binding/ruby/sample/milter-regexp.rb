# -*- coding: utf-8 -*-
#
# Copyright (C) 2010  Kouhei Sutou <kou@clear-code.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

require 'nkf'
require 'milter'

class MilterRegexp < Milter::ClientSession
  def initialize(context, regexp)
    super(context)
    @regexp = regexp
    @body = ''
  end

  def header(name, value)
    case name
    when /\ASubject\z/i
      if @regexp =~ NKF.nkf("-w", value)
        reject
      end
    end
  end

  def body(chunk)
    if @regexp =~ chunk
      reject
    end
    @body << chunk
  end

  def end_of_message
    if @regexp =~ @body
      reject
    end
  end
end

command_line = Milter::Client::CommandLine.new
command_line.run do |client, _options|
  client.register(MilterRegexp, /viagra/i)
end
