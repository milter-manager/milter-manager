module Milter
  class Client
    module Util
      def extract_address(address)
        return nil unless address
        address = address.dup.force_encoding("BINARY")
        address = address[/<([^<>]*)>/, 1] ||
          address[/[^\s<>]+@[^\s<>]+/] || address[/[^\s<>]+/] || address
        address
      end

      def address_spec(address)
        address = extract_address(address)
        if address.nil?
          nil
        else
          address.downcase
        end
      end

      #
      # Use this method for email address without angle bracket
      #
      def domain_name(address)
        return nil unless address
        address.slice(/\A(.+)@(.+)\z/, 2)
      end
    end
  end
end
