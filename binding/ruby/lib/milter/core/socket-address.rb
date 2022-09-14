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

require "ipaddr"
require "socket"

require "fiddle"

module Milter
  module SocketAddress
    class << self
      def resolve(address, address_size=nil)
        case address
        when IPv4, IPv6, Unix, Unknown
          return address
        end
        if address.is_a?(GLib::Pointer)
          address = Fiddle::Pointer.new(address).to_s(address_size)
        end
        if address.is_a?(String)
          addrinfo = Addrinfo.new(address)
        end
        if addrinfo.ipv4?
          IPv4.new(addrinfo.ip_address, addrinfo.ip_port)
        elsif addrinfo.ipv6?
          IPv6.new(addrinfo.ip_address, addrinfo.ip_port)
        elsif addrinfo.unix?
          Unix.new(addrinfo.unix_path)
        else
          Unknown.new
        end
      end
    end

    class IPv4
      def initialize(address, port)
        @addrinfo = Addrinfo.tcp(address, port)
      end

      def pack
        @addrinfo.to_s
      end

      def ==(other)
        other.is_a?(self.class) and pack == other.pack
      end

      def local?
        @addrinfo.ipv4_private?
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
        @ip_address ||= IPAddr.new(@addrinfo.ip_address)
      end
    end

    class IPv6
      def initialize(address, port)
        @addrinfo = Addrinfo.tcp(address, port)
      end

      def pack
        @addrinfo.to_s
      end

      def ==(other)
        other.is_a?(self.class) and pack == other.pack
      end

      def local?
        @addrinfo.ipv6_loopback? or @addrinfo.ipv6_linklocal?
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
        @ip_address ||= IPAddr.new(@addrinfo.ip_address)
      end
    end

    class Unix
      def initialize(path)
        @addrinfo = Addrinfo.unix(path)
      end

      def pack
        @addrinfo.to_s
      end

      def ==(other)
        other.is_a?(self.class) and pack == other.pack
      end

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
      def ==(other)
        other.is_a?(self.class)
      end

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
