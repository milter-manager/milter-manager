# Copyright (C) 2010  Kouhei Sutou <kou@clear-code.com>
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
  class AddressMatcher
    def initialize
      @local_addresses = []
      @remote_addresses = []
    end

    def local_address?(address)
      return false if unknown_address?(address)
      ip_address = address.to_ip_address
      return false if ip_address and custom_remote_address?(ip_address)
      return true if address.local?
      return true if ip_address and custom_local_address?(ip_address)
      false
    end

    def remote_address?(address)
      return false if unknown_address?(address)
      not local_address?(address)
    end

    def unknown_address?(address)
      address.unknown?
    end

    def add_local_address(address)
      @local_addresses << ensure_address(address)
    end

    def add_remote_address(address)
      @remote_addresses << ensure_address(address)
    end

    private
    def custom_local_address?(ip_address)
      @local_addresses.any? do |local_address|
        local_address.include?(ip_address)
      end
    end

    def custom_remote_address?(ip_address)
      @remote_addresses.any? do |remote_address|
        remote_address.include?(ip_address)
      end
    end

    def ensure_address(address)
      address = IPAddr.new(address) unless address.is_a?(IPAddr)
      address
    end
  end
end
