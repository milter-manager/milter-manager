#!/usr/bin/env ruby
#
# Copyright (C) 2019  Sutou Kouhei <kou@clear-code.com>
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

require "milter"

class MilterReplace < Milter::ClientSession
  def initialize(context, patterns)
    super(context)
    @patterns = patterns
    reset
  end

  def header(name, value)
    @headers << [name, value]
  end

  def body(chunk)
    @body << chunk
  end

  def end_of_message
    header_indexes = {}
    @headers.each do |name, value|
      header_indexes[name] ||= 0
      header_indexes[name] += 1
      @patterns.each do |pattern, replaced|
        replaced_value = value.gsub(pattern, replaced)
        if value != replaced_value
          change_header(name, header_indexes[name], replaced_value)
          break
        end
      end
    end

    @patterns.each do |pattern, replaced|
      replaced_body = @body.gsub(pattern, replaced)
      if @body != replaced_body
        replace_body(replaced_body)
      end
    end
  end

  def reset
    @headers = []
    @body = ""
  end
end

command_line = Milter::Client::CommandLine.new
command_line.run do |client, _options|
  client.register(MilterReplace, {
                    /viagra/i => "XXX",
                  })
end
