# Copyright (C) 2008-2010  Kouhei Sutou <kou@clear-code.com>
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

require 'ipaddr'

module Milter
  class Logger
    @@domain = "milter"
    class << self
      def domain
        @@domain
      end

      def domain=(domain)
        @@domain = domain
      end

      def error(message)
        default.log(:error, message, 1)
      end

      def critical(message)
        default.log(:critical, message, 1)
      end

      def message(message)
        default.log(:message, message, 1)
      end

      def warning(message)
        default.log(:warning, message, 1)
      end

      def debug(message)
        default.log(:debug, message, 1)
      end

      def info(message)
        default.log(:info, message, 1)
      end

      def statistics(message)
        default.log(:statistics, message, 1)
      end
    end

    def log(level, message, n_call_depth=nil)
      unless level.is_a?(Milter::LogLevelFlags)
        level = Milter::LogLevelFlags.const_get(level.to_s.upcase)
      end
      n_call_depth ||= 0
      file, line, info = caller[n_call_depth].split(/:(\d+):/, 3)
      ensure_message(message).each_line do |one_line_message|
        log_full(self.class.domain, level, file, line.to_i, info.to_s,
                 one_line_message.chomp)
      end
    end

    private
    def ensure_message(message)
      case message
      when nil
        ''
      when String
        message
      when Exception
        "#{message.class}: #{message.message}:\n#{message.backtrace.join("\n")}"
      else
        message.inspect
      end
    end
  end

  module SocketAddress
    class IPv4
      def local?
        bit1, bit2, bit3, bit4 = address.split(/\./).collect {|bit| bit.to_i}
        return true if bit1 == 127
        return true if bit1 == 10
        return true if bit1 == 172 and (16 <= bit2 and bit2 < 32)
        return true if bit1 == 192 and bit2 == 168
        false
      end

      def unknown?
        false
      end

      def to_ip_address
        @ip_address ||= IPAddr.new(address)
      end
    end

    class IPv6
      def local?
        abbreviated_before, abbreviated_after = address.split(/::/)
        bits_before = abbreviated_before.split(/:/)
        bits_after = (abbreviated_after || '').split(/:/)
        abbreviated_bits_size = 8 - bits_before.size - bits_after.size
        bits = bits_before + (["0"] * abbreviated_bits_size) + bits_after
        bits = bits.collect {|bit| bit.to_i(16)}
        return true if bits == [0, 0, 0, 0, 0, 0, 0, 0x0001]
        return true if bits[0] == 0xfe80
        false
      end

      def unknown?
        false
      end

      def to_ip_address
        @ip_address ||= IPAddr.new(address)
      end
    end

    class Unix
      def local?
        true
      end

      def unknown?
        false
      end

      def to_ip_address
        nil
      end
   end

    class Unknown
      def local?
        false
      end

      def unknown?
        true
      end

      def to_ip_address
        nil
      end
    end
  end

  class ProtocolAgent
    def set_macros(context, macros)
      macros.each do |name, value|
        set_macro(context, name, value)
      end
    end
  end

  module MacroNameNormalizer
    def normalize_macro_name(name)
      name.sub(/\A\{(.+)\}\z/, '\1')
    end
  end

  module MacroPredicates
    def authenticated?
      (self["auth_type"] or self["auth_authen"]) ? true : false
    end

    def postfix?
      (/\bPostfix\b/i =~ (self["v"] || '')) ? true : false
    end
  end
end
