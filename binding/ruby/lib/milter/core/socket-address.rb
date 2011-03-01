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

      def ipv4?
        true
      end

      def ipv6?
        false
      end

      def unix?
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

      def ipv4?
        false
      end

      def ipv6?
        true
      end

      def unix?
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

      def ipv4?
        false
      end

      def ipv6?
        false
      end

      def unix?
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

      def ipv4?
        false
      end

      def ipv6?
        false
      end

      def unix?
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
end
