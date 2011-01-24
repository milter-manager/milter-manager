# Copyright (C) 2010-2011  Kouhei Sutou <kou@clear-code.com>
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

module Milter::Manager
  class NetstatConnectionChecker
    def initialize(options={})
      @options = (options || {}).dup
      @database = nil
      @last_update = nil
      detect_netstat_command
    end

    def connected?(context)
      return true if @netstat_command.nil?
      return true unless context.smtp_server_address.local?
      address = context.smtp_client_address
      type = nil
      case address
      when Milter::SocketAddress::IPv4
        type = :tcp
      when Milter::SocketAddress::IPv6
        type = :tcp6
      end
      return true if type.nil?

      if address.port.zero?
        message = "[netstat][warning] " +
          "can't detect disconnected connection because " +
          "SMTP client port address is unknown: " +
          "<#{address}>"
        smtp_server_name = context.smtp_server_name
        message << ":<#{smtp_server_name}>" if smtp_server_name
        Milter::Logger.warning(message)
        return true
      end

      update_database
      state = @database[type]["#{address.address}:#{address.port}"]
      return false if state.nil? or state == "CLOSE_WAIT"
      true
    end

    def database_lifetime
      @options[:database_lifetime] || 5
    end

    def database_lifetime=(lifetime)
      @options[:database_lifetime] = lifetime
    end

    private
    def netstat
      result = `#{@netstat_command}`
      status = $?
      if status.success?
        result
      else
        Milter::Logger.error("[netstat][error] failed to run netstat: " +
                             "<#{@netstat_command}>: <#{result}>")
        ""
      end
    end

    def update_database
      return unless need_database_update?
      @database = {:tcp => {}, :tcp6 => {}}
      parse_netstat_result(netstat)
    end

    def need_database_update?
      return true if @database.nil?
      return true if @last_update.nil?
      Time.now - @last_update > database_lifetime
    end

    def parse_netstat_result(result)
      result.each_line do |line|
        next if line !~ /\Atcp/
        info = line.split
        protocol, recv_q, recv_q, local_address, foreign_address, state = info

        components = foreign_address.split(/[:.]/)
        target_port = components.pop
        if protocol == "tcp6"
          type = :tcp6
          target_address = components.join(":")
        else
          type = :tcp
          target_address = components.join(".")
        end
        @database[type]["#{target_address}:#{target_port}"] = state
      end
    end

    def detect_netstat_command
      @netstat_command = nil
      commands = ["env LANG=C netstat -n -W 2>&1",
                  "env LANG=C netstat -n 2>&1"]
      commands.each do |command|
        result = `#{command}`
        if $?.success?
          @netstat_command = command
          break
        end
      end
      if @netstat_command
        Milter::Logger.info("[netstat][detect] <#{@netstat_command}>")
      else
        inspected_commands = commands.collect do |command|
          "<#{command}>"
        end.join(" ")
        Milter::Logger.error("[netstat][not-available] #{inspected_commands}")
      end
    end
  end
end
