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
      return true if address.local?
      return true if custom_local_address?(address)
      false
    end

    def add_local_address(address)
      address = IPAddr.new(address) unless address.is_a?(IPAddr)
      @local_addresses << address
    end

    private
    def custom_local_address?(address)
      ip_address = address.to_ip_address
      return false if ip_address.nil?
      @local_addresses.any? do |local_address|
        local_address.include?(ip_address)
      end
    end
  end
end
