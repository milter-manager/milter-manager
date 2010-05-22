#!/usr/bin/env ruby
#
# Copyright (C) 2010  Kouhei Sutou <kou@clear-code.com>
#
# This library is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this library.  If not, see <http://www.gnu.org/licenses/>.

require 'syslog'
require 'milter'

class MilterReportPortSession < Milter::ClientSession
  def connect(host, address)
    @host = host
    @address = address
  end

  def end_of_message
    queue_id = @context.macros["i"]
    Syslog.info("host_name=%s address=%s port=%d queue_id=%s",
                @host, @address.address, @address.port, queue_id)
    accept
  end
end

command_line = Milter::Client::CommandLine.new

command_line.run do |client, _options|
  Syslog.open("milter-report-port",
              Syslog::LOG_PID | Syslog::LOG_CONS,
              Syslog::LOG_MAIL)
  client.register(MilterReportPortSession)
end

Syslog.close
