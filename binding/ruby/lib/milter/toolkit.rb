module Milter
  module SocketAddress
    class IPv4
      def local?
        bit1, bit2, bit3, bit4 = host.split(/\./).collect {|bit| bit.to_i}
        return true if bit1 == 127
        return true if bit1 == 10
        return true if bit1 == 172 and (16 <= bit2 and bit2 < 32)
        return true if bit1 == 192 and bit2 == 168
        false
      end
    end

    class IPv6
      def local?
        abbreviated_before, abbreviated_after = host.split(/::/)
        bits_before = abbreviated_before.split(/:/)
        bits_after = (abbreviated_after || '').split(/:/)
        abbreviated_bits_size = 8 - bits_before.size - bits_after.size
        bits = bits_before + (["0"] * abbreviated_bits_size) + bits_after
        bits = bits.collect {|bit| bit.to_i(16)}
        return true if bits == [0, 0, 0, 0, 0, 0, 0, 0x0001]
        return true if bits[0] == 0xfe80
        false
      end
    end

    class Unix
      def local?
        true
      end
    end
  end
end
