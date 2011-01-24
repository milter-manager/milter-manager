#!/usr/bin/env ruby
# -*- coding: utf-8 -*-
#
# Copyright (C) 2011  Kouhei Sutou <kou@clear-code.com>
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

require 'milter'

class MilterRejectNilSender < Milter::ClientSession
  def header(name, value)
    if name == "Sender" and /\bnil\b/ =~ value
      reject(:reason => "broken Sender header")
    end
  end
end

command_line = Milter::Client::CommandLine.new
command_line.run do |client, _options|
  client.register(MilterRejectNilSender)
end
