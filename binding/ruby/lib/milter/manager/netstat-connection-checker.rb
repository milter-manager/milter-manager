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
    class ConnectionInfo < Struct.new(:protocol,
                                      :local_ip_address, :local_port,
                                      :foreign_ip_address, :foreign_port,
                                      :state)
      def local_tcp_address
        "#{local_ip_address}:#{local_port}"
      end

      def foreign_tcp_address
        "#{foreign_ip_address}:#{foreign_port}"
      end
    end

    def initialize(options={})
      @options = (options || {}).dup
      @database = nil
      @last_update = nil
      detect_netstat_command
    end

    def connected?(context)
      return true unless context.smtp_server_address.local?
      info = connection_info(context.smtp_client_address)
      return true if info.nil?
      state = info.state
      return false if state.nil? or state == "CLOSE_WAIT"
      true
    end

    def smtp_server_interface_ip_address(client_address)
      info = connection_info(client_address, :retry => true)
      return nil if info.nil?
      return info.local_ip_address if info.state == "ESTABLISHED"
      nil
    end

    def smtp_server_interface_port(client_address)
      info = connection_info(client_address, :retry => true)
      return nil if info.nil?
      return info.local_port if info.state == "ESTABLISHED"
      nil
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

    def connection_info(address, options={})
      return nil if @netstat_command.nil?
      type = nil
      case address
      when Milter::SocketAddress::IPv4
        type = :tcp4
      when Milter::SocketAddress::IPv6
        type = :tcp6
      end
      return nil if type.nil?

      if address.port.zero?
        message = "[netstat][warning] " +
          "can't detect disconnected connection because " +
          "SMTP client port address is unknown: " +
          "<#{address}>"
        Milter::Logger.warning(message)
        return nil
      end

      tcp_address = "#{address.address}:#{address.port}"
      update_database
      info = @database[type][tcp_address]
      if info.nil? and options[:retry]
        purge_cache
        update_database
        info = @database[type][tcp_address]
      end
      info
    end

    def purge_cache
      @database = nil
      @last_update = nil
    end

    def update_database
      return unless need_database_update?
      @database = {:tcp4 => {}, :tcp6 => {}}
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
        protocol, recv_q, send_q, \
          local_tcp_address, foreign_tcp_address, state = info

        protocol = "tcp4" if protocol == "tcp"
        local_ip_address, local_port =
          parse_tcp_address(protocol, local_tcp_address)
        foreign_ip_address, foreign_port =
          parse_tcp_address(protocol, foreign_tcp_address)
        info = ConnectionInfo.new(protocol,
                                  local_ip_address, local_port,
                                  foreign_ip_address, foreign_port,
                                  state)
        @database[protocol.to_sym][info.foreign_tcp_address] = info
      end
    end

    def parse_tcp_address(protocol, tcp_address)
      components = tcp_address.split(/[:.]/)
      port = components.pop
      case protocol
      when "tcp6"
        ip_address = components.join(":")
      else
        ip_address = components.join(".")
      end
      [ip_address, port]
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
