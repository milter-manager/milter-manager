module Milter
  module Manager
    class Error < StandardError
    end

    class DecodeError < Error
    end

    class ConnectionError < Error
    end
  end
end
